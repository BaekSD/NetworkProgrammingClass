#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define RCVBUFSIZE	1024

#define EchoReq	01
#define FileUpReq	02
#define EchoRep	11
#define FileAck	12

int fting = 0;
int quit = 0;

void DieWithError(char *errorMessage);
void FTClient(int sock, int ftsock);

struct linked_list {
	char msg[RCVBUFSIZE];
	struct linked_list* next;
};

struct linked_list *list;

void *threadMain(void *args) {
	int sock = *((int*)args);
	int bytesRcvd = 0;
	char msgType = 0;
	char echoBuffer[RCVBUFSIZE];
	struct linked_list *tmp;

	while(1) {
		if((bytesRcvd = recv(sock, &msgType, sizeof(char), 0)) < 0) {
			if(quit) {
				break;
			}
			DieWithError("recv() failed or connection closed prematurely");
		}
		if(msgType != EchoRep) {
			if(quit) {
				break;
			}
			continue;
		}
		if((bytesRcvd = recv(sock, echoBuffer, RCVBUFSIZE-1, 0)) < 0) {
			if(quit) {
				break;
			}
			DieWithError("recv() failed or connection closed prematurely");
		}
		if(bytesRcvd == 0) {
			if(quit) {
				break;
			}
			printf("Disconnected");
			break;
		}
		echoBuffer[bytesRcvd] = '\0';
		if(!fting) {
			printf("\rmsg<- %s\n", echoBuffer);
			printf("msg-> ");
			fflush(stdout);
		} else {
			if(list == NULL) {
				list = (struct linked_list*)malloc(sizeof(struct linked_list));
				list->next = NULL;
				strcpy(list->msg, echoBuffer);
			} else {
				tmp = list;
				while(tmp->next != NULL) {
					tmp = tmp->next;
				}
				tmp->next = (struct linked_list*)malloc(sizeof(struct linked_list));
				tmp->next->next = NULL;
				strcpy(tmp->next->msg, echoBuffer);
			}
		}
	}

	return NULL;
}

int main(int argc, char *argv[]) {
	int sock, ftsock;
	struct sockaddr_in echoServAddr, echoServAddrFT;
	char portNum[10];
	unsigned short echoServPort;
	unsigned short echoServPortFT = 54567;
	char servIP[128];
	char echoString[RCVBUFSIZE];
	char echoBuffer[RCVBUFSIZE];
	unsigned int echoStringLen;
	int bytesRcvd, totalBytesRcvd;
	char msgType = 0;
	struct linked_list* tmp;

	//servIP = "127.0.0.1";
	echoServPort = 30405;

	pthread_t tid;

	if((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		DieWithError("socket() failed");
	}

	if((ftsock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		DieWithError("socket() failed");
	}

	printf("server ip : ");
	fgets(servIP, 127, stdin);
	servIP[strlen(servIP)-1] = '\0';
	fflush(stdin);

	printf("port : ");
	fgets(portNum, 9, stdin);
	echoServPort = atoi(portNum);
	fflush(stdin);

	memset(&echoServAddr, 0, sizeof(echoServAddr));
	echoServAddr.sin_family = AF_INET;
	echoServAddr.sin_addr.s_addr = inet_addr(servIP);
	echoServAddr.sin_port = htons(echoServPort);

	memset(&echoServAddrFT, 0, sizeof(echoServAddrFT));
	echoServAddrFT.sin_family = AF_INET;
	echoServAddrFT.sin_addr.s_addr = inet_addr(servIP);
	echoServAddrFT.sin_port = htons(echoServPortFT);

	if(connect(sock, (struct sockaddr*)&echoServAddr, sizeof(echoServAddr)) < 0) {
		DieWithError("connect() failed");
	}

	if(connect(ftsock, (struct sockaddr*)&echoServAddrFT, sizeof(echoServAddrFT)) < 0) {
		DieWithError("connect() failed");
	}

	if(pthread_create(&tid, NULL, threadMain, (void*)&sock) != 0) {
		DieWithError("pthread_create() failed");
	}

	while(1) {
		printf("msg-> ");
		fgets(echoString, RCVBUFSIZE-1, stdin);
		echoString[strlen(echoString)-1] = '\0';
		echoStringLen = strlen(echoString);

		if(strcmp(echoString, "FT") == 0) {
			fting = 1;
			FTClient(sock, ftsock);
			fting = 0;
			while(list != NULL) {
				printf("msg<- %s\n",list->msg);
				tmp = list;
				list = list->next;
				tmp->next = NULL;
				free(tmp);
			}
		} else {
			msgType = EchoReq;
			if(strcmp(echoString, "/quit") == 0) {
				quit = 1;
			}
			if(send(sock, &msgType, sizeof(char), 0) != sizeof(char)) {
				DieWithError("send() sent a different number of bytes than expected");
			}
			if(send(sock, echoString, echoStringLen, 0) != echoStringLen) {
				DieWithError("send() sent a different number of bytes than expected");
			}
			if(strcmp(echoString, "/quit") == 0) {
				break;
			}
		}
	}

	close(sock);
	exit(0);
}
