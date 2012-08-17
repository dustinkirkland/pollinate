/*

anerd: Asynchronous Network Exchange Randomness Daemon

Copyright 2012 Dustin Kirkland <kirkland@ubuntu.com>

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

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
#include <pwd.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#define DEFAULT_SIZE 64
#define DEFAULT_DEVICE "/dev/urandom"
#define DEFAULT_PORT 26373
#define DEFAULT_INTERVAL 60
#define DAEMON_USER "daemon"

/* Daemonize process properly */
void daemonize();

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
	/* Open the random device */
	if ((fp = fopen(device, "a+")) == NULL) {
		perror("fopen");
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
	/* Make process into a daemon */
	daemonize();
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
		syslog(LOG_INFO, "Received [%d] bytes from [%s:%d]\n", bytes_read, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
		fflush(stdout);
		/* Mix incoming entropy + salt into pool */
		fwrite(data, sizeof(char), bytes_read, fp);
		fwrite(&salt, sizeof(double), 1, fp);
		fflush(fp);
		/* Obtain some entropy for transmission */
		fread(data, sizeof(char), bytes_read, fp);
		/* Return the favor, sending entropy back to the initiator */
		sendto(sock, data, strlen(data), 0, (struct sockaddr *)&client_addr, sizeof(struct sockaddr));
		syslog(LOG_INFO, "Transmit [%d] bytes to [%s:%d]\n", bytes_read, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
	}
	/* Should never get here; clean up if we do */
	free(data);
	close(sock);
	fclose(fp);
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
	char *data;
	FILE *fp;
	int broadcast = 1;
	/* Allocate and zero a data buffer to the chosen size */
	if ((data = calloc(size, sizeof(char))) == NULL) {
		perror("calloc");
		exit(1);
	}
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
	server_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	bzero(&(server_addr.sin_zero),8);
	if ((fp = fopen(device, "r")) == NULL) {
		/* Error reading entropy; skip this round */
		perror("fopen");
		close(sock);
		exit(1);
	}
	/* Make process into a daemon */
	daemonize();
	/* Periodically trigger a network entropy exchange */
	while (interval > 0) {
		/* Donate some entropy to the local networks */
		if (fread(data, 1, size, fp) > 0) {
			syslog(LOG_INFO, "Donated  [%d] bytes to [%s:%d]\n", size, inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
			sendto(sock, data, size, 0, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));
		} else {
			perror("fread");
		}
		/* Clean-up */
		sleep(interval);
	}
	/* Should never get here; clean up if we do */
	close(sock);
	free(data);
	fclose(fp);
	return 0;
}

int main(int argc, char *argv[]) {
	int arg;
	int interval = DEFAULT_INTERVAL;
	int size = DEFAULT_SIZE;
	int port = DEFAULT_PORT;
	char *device = DEFAULT_DEVICE;
	/* Getopt command-line argument handling */
	while ((arg = getopt(argc, argv, "d:i:p:s:")) != -1) {
		switch (arg) {
			case 'd':
				device = optarg;
				break;
			case 'i':
				if (isdigit(optarg[0])) {
					interval = atoi(optarg);
				} else {
					fprintf(stderr, "Option -i requires a integer argument.\n");
					return 1;
				}
				break;
			case 'p':
				if (isdigit(optarg[0])) {
					port = atoi(optarg);
				} else {
					fprintf(stderr, "Option -p requires a integer argument.\n");
					return 1;
				}
				break;
			case 's':
				if (isdigit(optarg[0])) {
					size = atoi(optarg);
				} else {
					fprintf(stderr, "Option -p requires a integer argument.\n");
					return 1;
				}
				break;
			case '?':
				if (
						(optopt == 'd') || (optopt == 'i') ||
						(optopt == 'p') || (optopt == 's')) {
					fprintf(stderr, "Option -%c requires an argument.\n", optopt);
				} else {
					fprintf(stderr, "Unknown option -%c.\n", optopt);
				}
				return 1;
			default:
				abort();
		}
	}
	/* Set up syslog */
	openlog ("anerd", LOG_PERROR | LOG_CONS | LOG_PID | LOG_NDELAY, LOG_DAEMON);
	/* fork off process */
	int pid = fork();
	if (pid == 0) {
		/* if process is child */
		/* Fork a client process */
		anerd_client(device, size, port, interval);
	} else if (pid > 0) {
		/* if process is parent */
		/* fork off another process */
		int pid2 = fork();
		if (pid2 == 0) {
			/* if process is child */
			/* Fork a server process */
			anerd_server(device, size, port);
		} else if (pid2 > 0) {
			/* if process is parent */
			/* exit, freeing terminal that called program */
			exit(0);
		} else if (pid2 < 0) {
			/* if the second fork() call failed */
			/* exit because of error */
			exit(1);
		}
	} else if (pid < 0) {
		/* if the fork() call failed */
		/* exit because of error */
		exit(1);
	}
}

void daemonize() {
	/* Drop user priv if run as root */
	if ((getuid() == 0) || (geteuid == 0)) {
		struct passwd *pw = getpwnam(DAEMON_USER);
		if (pw) {
			syslog(LOG_NOTICE, "Setting User To: " DAEMON_USER);
			setuid(pw->pw_uid);
		}
	}
	/* Change current directory to / so current directory is not locked */
	if ((chdir("/")) < 0) {
		perror("chdir() failed");
		exit(1);
	}
	/* Change file mode mask */
	umask(0);
	/* Run setsid() so that daemon is no longer child of spawning process */
	if (setsid() < 0) {
		perror("setsid() failed");
		exit(1);
	}
	/* Close stdin, stdout, stderr */
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
}
