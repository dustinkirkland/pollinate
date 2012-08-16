/*
    anerd: Asynchronous Network Exchange Randomness Daemon
    Copyright (C) 2012 Dustin Kirkland <kirkland@ubuntu.com>

    Authors: Dustin Kirkland <kirkland@ubuntu.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define DEFAULT_SIZE 64
#define DEFAULT_DEVICE "/dev/urandom"
#define DEFAULT_PORT 26373
#define DEFAULT_INTERVAL 60

/*
anerd server:
 - Listen for communications on a UDP socket.
 - Take any received input, salt with a bit of local randomness (perhaps the
   time in microseconds between transmissions), and add to the entropy pool.
 - Transmit back to the initiator the same number of bytes of randomness.
*/
int anerd_server(char *device, int size, int port) {
	int sock;
	int addr_len, bytes_read;
	double salt;
	int last_usec, this_usec;
	char *data;
	struct sockaddr_in server_addr, client_addr;
	struct timeval tv;
	struct timezone tz;
	FILE *fp;
	/* Open the UDP socket */
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("Socket");
		exit(1);
	}
	/* Allocate and zero a data buffer to the chosen size */
	if ((data = calloc(size, sizeof(char))) == NULL) {
		perror("calloc");
		exit(1);
	}
	/* Set up and bind the socket */
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(server_addr.sin_zero),8);
	if (bind(sock,(struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
		perror("Bind");
		exit(1);
	}
	addr_len = sizeof(struct sockaddr);
	/* Seed the local, time-based salt; peers won't know this */
	gettimeofday(&tv, &tz);
	last_usec = 1000000 * tv.tv_usec + tv.tv_usec;
	while (1) {
		/* Receive data over our UDP socket */
		bytes_read = recvfrom(sock, data, size, 0, (struct sockaddr *)&client_addr, &addr_len);
		data[bytes_read] = '\0';
		/* Update local timestamp, generate new salt */
		gettimeofday(&tv, &tz);
		this_usec = 1000000 * tv.tv_usec + tv.tv_usec;
		salt = last_usec * this_usec;
		last_usec = this_usec;
		/* Logging/debug message */
		printf("Received [%d] bytes from [%s:%d]\n", bytes_read, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
		fflush(stdout);
		/* Mix incoming entropy + salt into pool */
		if ((fp = fopen(device, "a+")) == NULL) {
			perror("fopen");
			/* Error reading/writing entropy; skip this round */
			continue;
		}
		fwrite(data, sizeof(char), bytes_read, fp);
		fwrite(&salt, sizeof(double), 1, fp);
		/* Obtain some entropy for transmission */
		fread(data, sizeof(char), bytes_read, fp);
		fclose(fp);
		/* Return the favor, sending entropy back to the initiator */
		sendto(sock, data, strlen(data), 0, (struct sockaddr *)&client_addr, sizeof(struct sockaddr));
		printf("Transmit [%d] bytes to [%s:%d]\n", bytes_read, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
	}
	/* Should never get here; clean up if we do */
	free(data);
	close(sock);
	return 0;
}

/*
anerd client:
 - broadcast some randomness to the local network on the anerd UDP port
 - this is intended to "stir the pot", kicking up some randomness
 - it trigger anerd exchanges with any anerd servers on the network
*/
int anerd_client(char *device, int size, int port, int interval) {
	int sock;
	struct sockaddr_in server_addr;
	struct hostent *host;
	char *data;
	FILE *fp;
	int broadcast = 1;
	/* Set up the broadcast address */
	if ((host = gethostbyname("255.255.255.255")) == NULL) {
		perror("gethostbyname");
		exit(1);
	}
	/* Allocate and zero a data buffer to the chosen size */
	if ((data = calloc(size, sizeof(char))) == NULL) {
		perror("calloc");
		exit(1);
	}
	/* Periodically trigger a network entropy exchange */
	while (interval > 0) {
		/* Setup the UDP socket */
		if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
			perror("socket");
		}
		/* Configure the socket for broadcast */
		if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof broadcast) == -1) {
			perror("setsockopt (SO_BROADCAST)");
		}
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(port);
		server_addr.sin_addr = *((struct in_addr *)host->h_addr);
		bzero(&(server_addr.sin_zero),8);
		if ((fp = fopen(device, "r")) == NULL) {
			perror("fopen");
			close(sock);
			/* Error reading entropy; skip this round */
			continue;
		}
		fread(data, 1, size, fp);
		fclose(fp);
		sendto(sock, data, size, 0, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));
		/* Donate some entropy to the local network pool,
		   and hopefully trigger some responses */
		printf("Donating [%d] bytes to [%s:%d]\n", size, inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
		/* Clean-up */
		close(sock);
		sleep(interval);
	}
	/* Should never get here; clean up if we do */
	free(data);
	return 0;
}

int main(int argc, char *argv[]) {
	int i;
	int interval = DEFAULT_INTERVAL;
	int size = DEFAULT_SIZE;
	int port = DEFAULT_PORT;
	char *device = DEFAULT_DEVICE;
	/* Very naive command-line argument handling */
	for (i=0; i<argc; i++) {
		if (strncmp(argv[i], "-d", 2) == 0) {
			device = argv[++i];
		} else if (strncmp(argv[i], "-i", 2) == 0) {
			interval = atoi(argv[++i]);
		} else if (strncmp(argv[i], "-p", 2) == 0) {
			port = atoi(argv[++i]);
		} else if (strncmp(argv[i], "-s", 2) == 0) {
			size = atoi(argv[++i]);
		}
	}
	if (fork() == 0) {
		/* Fork a client process */
		anerd_client(device, size, port, interval);
	} else {
		/* Fork a server process */
		anerd_server(device, size, port);
	}
}
