#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define MAXPENDING	5

void DieWithError(char *errorMessage);
void HandleTCPClient(int clntSocket);

void *threadMain(void *args) {
	int *csock = (int*)args;

	usleep(1000);

	HandleTCPClient(*csock);
	free(csock);
	pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
	int servSock;
	int *clntSock;
	struct sockaddr_in echoServAddr;
	struct sockaddr_in echoClntAddr;
	unsigned short echoServPort = 30405;
	unsigned int clntLen;
	pthread_t tid;

	if((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		DieWithError("socket() failed");
	}

	memset(&echoServAddr, 0, sizeof(echoServAddr));
	echoServAddr.sin_family = AF_INET;
	echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	echoServAddr.sin_port = htons(echoServPort);

	if(bind(servSock, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr)) < 0) {
		DieWithError("bind() failed");
	}

	if(listen(servSock, MAXPENDING) < 0) {
		DieWithError("listen() failed");
	}

	for(;;) {
		clntSock = (int*)malloc(sizeof(int));
		clntLen = sizeof(echoClntAddr);

		if((*clntSock = accept(servSock, (struct sockaddr*)&echoClntAddr, &clntLen)) < 0) {
			DieWithError("accept() failed");
		}

		if(pthread_create(&tid, NULL, threadMain, (void*)clntSock) != 0) {
			DieWithError("pthread_create() failed");
		}

		printf("client ip : %s\n", inet_ntoa(echoClntAddr.sin_addr));
		printf("port : %d\n", ntohs(echoClntAddr.sin_port));
	}
}
