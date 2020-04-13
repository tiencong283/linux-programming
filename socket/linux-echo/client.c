#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>          
#include <sys/socket.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

// limit the number of pending connections
// possible limit at /proc/sys/net/core/somaxconn
#define BACKLOG 4
#define BUFSIZE 1024

char const* host = NULL;
char const* port = NULL;
char const* prompt = "> ";

int main(int argc, char const** argv){
	int ret = 0;
	struct addrinfo hints;
	struct addrinfo* addr;
	
	if (argc != 3){
		printf("Usage: %s <HOST> <PORT>\n", argv[0]);
		return 0;
	}
	host = argv[1];
	port = argv[2];

	// disable buffering
	setvbuf(stdout, NULL, _IONBF, 0);

	// create socket	
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0){
		printf("ERROR: cannot create socket, errno=%d\n", errno);
		return 1;
	}
	// map hostname and service to addr
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_NUMERICSERV | AI_PASSIVE;	// numeric port, do not translate and wildcard address, serverIP ignored ??

	if (getaddrinfo(host, port, &hints, &addr) != 0){
		perror("ERROR: getaddrinfo");
		return 1;
	}
	// connect to server
	if (connect(sock, addr->ai_addr, addr->ai_addrlen) != 0){
		freeaddrinfo(addr);
		close(sock);
		return 1;
	}
	freeaddrinfo(addr);
	printf("Connected at (%s, %s)\n", host, port);
	
	char buffer[BUFSIZE];
	int bufLen = 0;
	do {	// read from user's input
		printf("%s ", prompt);
		ret = read(0, buffer, BUFSIZE-1);
		if (ret == -1){ // IO errors
			perror("ERROR: read");
			close(sock);
			return 1;
		}
		buffer[ret] = '\x00';
		buffer[strcspn(buffer, "\n")] = '\x00';
		bufLen = strlen(buffer);
		if (bufLen == 0){
			break;
		}
		
		// send to server
		ret = send(sock, buffer, bufLen, 0);
		if (ret == -1){
			perror("ERROR: send");
			close(sock);
			return 1;
		}
		if (ret == 0){
			printf("ERROR: The peer server closed\n");
			break;
		}
		if (ret != bufLen){
			printf("Warning: the message not sent completely (%d/%d)", ret, bufLen);
		}
		// read back from server
		ret = recv(sock, buffer, BUFSIZE - 1, 0);
		if (ret == -1){
			perror("ERROR: recv");
			close(sock);
			return 1;
		}
		if (ret == 0){
			printf("ERROR: The peer server closed\n");
			break;
		}
		buffer[ret] = '\x00'; 
		printf("msg: '%s'\n", buffer);
	} while (1);

	close(sock);
	return 0;
}
