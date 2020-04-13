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
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>

// limit the number of pending connections
// possible limit at /proc/sys/net/core/somaxconn
#define BACKLOG 4
// maximum number of concurrent requests
#define CREQ_MAX 4
#define BUFSIZE 1024

char const* host = NULL;
char const* port = NULL;

typedef struct clientStruct {
	int pid;
	int sock;
	struct sockaddr_in addr;
	socklen_t addrlen;
} clientStruct;

clientStruct* clients[CREQ_MAX];
int numOfClients = 0;
int clientAt = 0;

// log new client's connection
void onNewClient(clientStruct const* c){
	char buffer[BUFSIZE];
	memset(buffer, 0, BUFSIZE);
	if (inet_ntop(AF_INET, &c->addr.sin_addr, buffer, BUFSIZE-1) == NULL){
		printf("Warning: cannot get host of new connection\n");	
		return;
	}
	printf("New connection from (%s, %d)\n", buffer, ntohs(c->addr.sin_port));
}

void closePeerSock(clientStruct* c){
	printf("Disconnected connection from (%s, %d)\n", inet_ntoa(c->addr.sin_addr), ntohs(c->addr.sin_port));
	close(c->sock);
}

// echo back the message from client
void onRequest(){
	clientStruct* c = clients[clientAt];
	int sock = c->sock;
	char buffer[BUFSIZE];
	int ncom;
	for(;;){
		ncom = recv(sock, buffer, BUFSIZE-1, 0);
		if (ncom < 0){
			perror("recv");
			break;
		}
		if (ncom == 0){
			break;
		}
		ncom = send(sock, buffer, ncom, 0);
		if (ncom < 0){
			perror("send");
			break;
		}
		if (ncom == 0){
			break;
		}
	}
	closePeerSock(c);
}

void onInterruptKey(int sig){
	char buffer[BUFSIZE];
	int nread;
	int res = 0;
	if (numOfClients != 0){
		printf("Do you want to terminate all active connections (y/n)? ");
		if((nread=read(0, buffer, BUFSIZE - 1)) > 0){
			buffer[nread] = '\x00';
			buffer[strcspn(buffer, "\n")] = '\x00';
			if (!strcmp(buffer, "y") || !strcmp(buffer, "yes")){
				res = 1;
			}
		}
		if (res){	// ok
			for (int i=0; i<CREQ_MAX; ++i){
				if (clients[i] != NULL){
					if (kill(clients[i]->pid, SIGTERM) == -1){	// first attempt
						kill(clients[i]->pid, SIGKILL);	// last resort
					}
					free(clients[i]);
					clients[i] = NULL;
				}
			}
		}
	} else {
		printf("\n");
	}
	exit(0);
}

// handle control-c signals
int setSignals(){
	void* old = NULL;
	if ((old=signal(SIGINT, onInterruptKey)) == SIG_ERR){
		perror("signal");
		return 1;
	}
	return 0;
}

// entry point
int main(int argc, char const** argv){
	int ret = 0;
	int reuse = 1;
	struct addrinfo hints;
	struct addrinfo* addr;

	if (argc != 3){
		printf("Usage: %s <HOST> <PORT>\n", argv[0]);
		return 0;
	}
	host = argv[1];
	port = argv[2];
	
	// disable IO buffering libc
	setbuf(stdout, NULL);

	if (setSignals() != 0){
		return 1;
	}
	// create socket	
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0){
		perror("sock");
		return 1;
	}
	// bypass 'address already in use' restriction
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

	// map hostname and service to addr
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_NUMERICSERV | AI_PASSIVE;	// numeric port, do not translate and wildcard address, serverIP ignored ??

	if ((ret=getaddrinfo(host, port, &hints, &addr)) != 0){
		printf("getaddrinfo: %s\n", gai_strerror(ret));
		return 1;
	}
	// binding
	if (bind(sock, addr->ai_addr, addr->ai_addrlen) != 0){
		perror("bind");
		close(sock);
		freeaddrinfo(addr);
		return 1;
	}
	freeaddrinfo(addr);

	// listen
	if (listen(sock, BACKLOG) != 0){
		perror("listen");
		close(sock);
		return 1;
	}
	printf("Start listening at (%s,%s)\n", host, port);	
	
	// accept
	clientStruct* cp;
	int csock;
	int pid;
	int termAt;
	for(;;){	// wait for connections
		if (numOfClients >= CREQ_MAX){
			pid = waitpid(-1, NULL, WNOHANG);
			if (pid == 0){	// all busy then discard incomming connections
				csock = accept(sock, (struct sockaddr *)&cp->addr, &cp->addrlen);
				close(csock);
				continue;
			}
			termAt = 0;
			while(clients[termAt]->pid != pid){
				termAt += 1;
			}
			free(clients[termAt]);
			clients[termAt] == NULL;
			numOfClients -= 1;
		}

		cp = (clientStruct*)malloc(sizeof(clientStruct));
		if (cp == NULL){
			perror("malloc");
			close(sock);
			return 1;
		}
		memset(cp, 0, sizeof(clientStruct));
		cp->addrlen = sizeof(cp->addr);
		csock = accept(sock, (struct sockaddr *)&cp->addr, &cp->addrlen); 
		if (csock == -1){
			perror("accept");
			close(sock);
			return 1;
		}
		cp->sock = csock;
		onNewClient(cp);
		
		clientAt = 0;
		while (clients[clientAt] != NULL){ // find free hole
			clientAt += 1;
		}	
		clients[clientAt] = cp;
		numOfClients += 1;
		// open new process for handling the request
		if ((pid = fork()) < 0){
			perror("fork");
			close(sock);
			return 1;
		}
		if (pid == 0){	// child
			cp->pid = getpid();
			signal(SIGINT, SIG_IGN);	// ignore interrupt signals
			onRequest();
			break;
		} else {	// parent
			cp->pid = pid;
			close(csock);	// decrement client sock's reference
		}
	}
	close(sock);
	return 0;
}
