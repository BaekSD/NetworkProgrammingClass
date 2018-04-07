#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define RCVBUFSIZE	1024

void DieWithError(char *errorMessage);

void HandleTCPClient(int clntSocket) {
	char echoBuffer[RCVBUFSIZE];
	char echoString[RCVBUFSIZE];
	int recvMsgSize, echoStringLen;

	if((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE-1, 0)) < 0) {
		DieWithError("recv() failed");
	}
		
	echoBuffer[recvMsgSize] = '\0';
	printf("msg<- %s\n",echoBuffer);

	strcpy(echoBuffer, "hi");

	if(send(clntSocket, echoBuffer, strlen(echoBuffer), 0) != strlen(echoBuffer)) {
		DieWithError("send() sent a different number of bytes than expected");
	}
	printf("msg-> %s\n",echoBuffer);

	while(1) {
		if((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE-1, 0)) < 0) {
			DieWithError("recv() failed");
		}
		echoBuffer[recvMsgSize] = '\0';
		if(strcmp(echoBuffer, "/quit") == 0 || recvMsgSize == 0) {
			printf("Disconnected\n");
			break;
		}

		printf("msg<- %s\n", echoBuffer);
		strcpy(echoString, echoBuffer);

		if(send(clntSocket, echoString, strlen(echoString), 0) != strlen(echoString)) {
			DieWithError("send() sent a different number of bytes than expected");
		}
		printf("msg-> %s\n", echoString);
	}

	close(clntSocket);
}
