#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

#define RCVBUFSIZE	32

void DieWithError(char *errorMessage);

void HandleTCPClient(int clntSocket) {
	char echoBuffer[RCVBUFSIZE];
	int recvMsgSize;
	FILE *f;

	if((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE-1, 0)) < 0) {
		DieWithError("recv() failed");
	}
	
	echoBuffer[recvMsgSize] = '\0';
	printf("Received: %s",echoBuffer);
	f = fopen("echo_history.log","a");
	fprintf(f,"%s",echoBuffer);

	while(recvMsgSize > 0) {
		if(send(clntSocket, echoBuffer, recvMsgSize, 0) != recvMsgSize) {
			DieWithError("send() failed");
		}
		if((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE-1, 0)) < 0) {
			DieWithError("recv() failed");
		}
		
		echoBuffer[recvMsgSize] = '\0';
		printf("%s",echoBuffer);
		fprintf(f,"%s",echoBuffer);
	}
	printf("\n");
	fprintf(f,"\n");

	fclose(f);
	close(clntSocket);
}
