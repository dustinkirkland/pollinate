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

int anerd_server(char *device, int size, int port) {
	int sock;
	int addr_len, bytes_read;
	double salt;
	int last_usec;
	char *data;
	struct sockaddr_in server_addr, client_addr;
	struct timeval tv;
	struct timezone tz;
	FILE *fp;
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("Socket");
		exit(1);
	}
	if ((data = calloc(size, sizeof(char))) == NULL) {
		perror("calloc");
		exit(1);
	}
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = INADDR_ANY;
	bzero(&(server_addr.sin_zero),8);
	if (bind(sock,(struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
		perror("Bind");
		exit(1);
	}
	addr_len = sizeof(struct sockaddr);
	fflush(stdout);
	gettimeofday(&tv, &tz);
	last_usec = tv.tv_usec;
	while (1) {
		bytes_read = recvfrom(sock, data, size, 0, (struct sockaddr *)&client_addr, &addr_len);
		data[bytes_read] = '\0';
		gettimeofday(&tv, &tz);
		salt = last_usec * tv.tv_usec;
		printf("Received [%d] bytes from [%s:%d]\n", bytes_read, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
		fflush(stdout);
		if ((fp = fopen(device, "a+")) == NULL) {
			perror("fopen");
			continue;
		}
		fwrite(data, sizeof(char), bytes_read, fp);
		fwrite(&salt, sizeof(double), 1, fp);
		fread(data, sizeof(char), bytes_read, fp);
		fclose(fp);
		sendto(sock, data, strlen(data), 0, (struct sockaddr *)&client_addr, sizeof(struct sockaddr));
		printf("Transmit [%d] bytes to [%s:%d]\n", bytes_read, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
	}
	free(data);
	close(sock);
	return 0;
}

int anerd_client(char *device, int size, int port, int interval) {
	int sock;
	struct sockaddr_in server_addr;
	struct hostent *host;
	char *data;
	FILE *fp;
	int broadcast = 1;
	if ((host = gethostbyname("255.255.255.255")) == NULL) {
		perror("gethostbyname");
		exit(1);
	}
	if ((data = calloc(size, sizeof(char))) == NULL) {
		perror("calloc");
		exit(1);
	}
	while (interval > 0) {
		if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
			perror("socket");
		}
		if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof broadcast) == -1) {
			perror("setsockopt (SO_BROADCAST)");
		}
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(port);
		server_addr.sin_addr = *((struct in_addr *)host->h_addr);
		bzero(&(server_addr.sin_zero),8);
		fp = fopen(device, "r");
		fread(data, 1, size, fp);
		fclose(fp);
		sendto(sock, data, size, 0, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));
		printf("Donating [%d] bytes to [%s:%d]\n", size, inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
		close(sock);
		sleep(interval);
	}
	return 0;
}

int main(int argc, char *argv[]) {
	int i;
	int interval = DEFAULT_INTERVAL;
	int size = DEFAULT_SIZE;
	int port = DEFAULT_PORT;
	char *device = DEFAULT_DEVICE;
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
		anerd_client(device, size, port, interval);
	} else {
		anerd_server(device, size, port);
	}
}
