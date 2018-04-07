#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define MAXPENDING	5

struct linked_list {
	int *clntSock;
	int *ftSock;
	struct linked_list *next;
	struct linked_list *prev;
};

struct linked_list *list;

void DieWithError(char *errorMessage);
void HandleTCPClient(struct linked_list* list);

void *threadMain(void *args) {
	struct linked_list *l = (struct linked_list*)args;

	usleep(1000);

	HandleTCPClient(l);
	
	free(l->clntSock);
	free(l->ftSock);
	l->prev->next = l->next;
	l->next->prev = l->prev;
	if(l->next == l) {
		free(l);
		list = NULL;
	} else {
		free(l);
	}
	pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
	int servSock;
	int servSockFT;
	int *clntSock;
	int *clntSockFT;
	struct sockaddr_in echoServAddr;
	struct sockaddr_in echoClntAddr;
	struct sockaddr_in echoServAddrFT;
	struct sockaddr_in echoClntAddrFT;
	unsigned short echoServPort = 30405;
	unsigned short echoServPortFT = 54567;
	unsigned int clntLen;
	pthread_t tid;
	struct linked_list* tmp = NULL;

	if((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		DieWithError("socket() failed");
	}
	if((servSockFT = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		DieWithError("socket() failed");
	}

	memset(&echoServAddr, 0, sizeof(echoServAddr));
	echoServAddr.sin_family = AF_INET;
	echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	echoServAddr.sin_port = htons(echoServPort);

	memset(&echoServAddrFT, 0, sizeof(echoServAddrFT));
	echoServAddrFT.sin_family = AF_INET;
	echoServAddrFT.sin_addr.s_addr = htonl(INADDR_ANY);
	echoServAddrFT.sin_port = htons(echoServPortFT);

	if(bind(servSock, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr)) < 0) {
		DieWithError("bind() failed");
	}

	if(bind(servSockFT, (struct sockaddr *)&echoServAddrFT, sizeof(echoServAddrFT)) < 0) {
		DieWithError("bind() failed");
	}

	if(listen(servSock, MAXPENDING) < 0) {
		DieWithError("listen() failed");
	}

	if(listen(servSockFT, MAXPENDING) < 0) {
		DieWithError("listen() failed");
	}

	for(;;) {
		clntSock = (int*)malloc(sizeof(int));
		clntLen = sizeof(echoClntAddr);

		if((*clntSock = accept(servSock, (struct sockaddr*)&echoClntAddr, &clntLen)) < 0) {
			DieWithError("accept() failed");
		}

		clntSockFT = (int*)malloc(sizeof(int));
		clntLen = sizeof(echoClntAddrFT);

		if((*clntSockFT = accept(servSockFT, (struct sockaddr*)&echoClntAddrFT, &clntLen)) < 0) {
			DieWithError("accept() failed");
		}
		
		if(list == NULL) {
			list = (struct linked_list*)malloc(sizeof(struct linked_list));
			list->next = list;
			list->prev = list;
			list->clntSock = clntSock;
			list->ftSock = clntSockFT;
			tmp = list;
		} else {
			tmp = (struct linked_list*)malloc(sizeof(struct linked_list));
			tmp->next = list;
			tmp->prev = list->prev;
			tmp->clntSock = clntSock;
			tmp->ftSock = clntSockFT;
			list->prev->next = tmp;
			list->prev = tmp;
		}
		
		if(pthread_create(&tid, NULL, threadMain, (void*)tmp) != 0) {
			DieWithError("pthread_create() failed");
		}

		printf("client ip : %s\n", inet_ntoa(echoClntAddr.sin_addr));
		printf("port : %d\n", ntohs(echoClntAddr.sin_port));
	}
}
