#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define RCVBUFSIZE	1024

void DieWithError(char *errorMessage);

int main(int argc, char *argv[]) {
	int sock;
	struct sockaddr_in echoServAddr;
	char portNum[10];
	unsigned short echoServPort;
	char servIP[128];
	char echoString[RCVBUFSIZE];
	char echoBuffer[RCVBUFSIZE];
	unsigned int echoStringLen;
	int bytesRcvd, totalBytesRcvd;

	//servIP = "127.0.0.1";
	echoServPort = 30405;

	if((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
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

	if(connect(sock, (struct sockaddr*)&echoServAddr, sizeof(echoServAddr)) < 0) {
		DieWithError("connect() failed");
	}

	strcpy(echoString, "hello\0");
	printf("msg-> %s\n",echoString);

	while(1) {
		echoStringLen = strlen(echoString);
		if(send(sock, echoString, echoStringLen, 0) != echoStringLen) {
			DieWithError("send() sent a different number of bytes than expected");
		}
		if(strcmp(echoString, "/quit") == 0) {
			break;
		}
		if((bytesRcvd = recv(sock, echoBuffer, RCVBUFSIZE-1, 0)) < 0) {
			DieWithError("recv() failed or connection closed prematurely");
		}
		if(bytesRcvd == 0) {
			printf("Disconnected\n");
		}
		echoBuffer[bytesRcvd] = '\0';
		printf("msg<- %s\n",echoBuffer);

		printf("msg-> ");
		fgets(echoString, RCVBUFSIZE-1, stdin);
		echoString[strlen(echoString)-1] = '\0';
	}

	close(sock);
	exit(0);
}
