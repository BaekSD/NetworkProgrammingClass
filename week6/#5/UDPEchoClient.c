#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define RCVBUFSIZE	1024

#define ConnectionReq	00
#define EchoReq	01
#define FileUpReq	02
#define ConnectionRep	10
#define EchoRep	11
#define FileAck	12

void DieWithError(char *errorMessage);
void FTClient(int sock, struct sockaddr_in serv_addr);

int main(int argc, char *argv[]) {
	int sock;
	struct sockaddr_in serv_addr;
	struct sockaddr_in from_addr;
	char portNum[10];
	unsigned short echoServPort;
	char servIP[128];
	char echoString[RCVBUFSIZE];
	char echoBuffer[RCVBUFSIZE];
	unsigned int echoStringLen;
	int bytesRcvd, totalBytesRcvd;
	char msgType = 0;
	socklen_t addr_size;

	//servIP = "127.0.0.1";
	echoServPort = 30405;

	if((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
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

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(servIP);
	serv_addr.sin_port = htons(echoServPort);

	strcpy(echoString, "hello");
	printf("msg-> %s\n",echoString);
	
	echoStringLen = strlen(echoString);

	msgType = ConnectionReq;
	if(sendto(sock, &msgType, sizeof(char), 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) != sizeof(char)) {
		DieWithError("send() sent a different number of bytes than expected");
	}

	if(sendto(sock, echoString, echoStringLen, 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) != echoStringLen) {
		DieWithError("send() sent a different number of bytes than expected");
	}
	addr_size = sizeof(from_addr);
	if((bytesRcvd = recvfrom(sock, &msgType, sizeof(char), 0, (struct sockaddr*)&from_addr, &addr_size)) < 0) {
		DieWithError("recv() failed or connection closed prematurely");
	}
	if(msgType != ConnectionRep) {
		DieWithError("Bad request...");
	}
	if((bytesRcvd = recvfrom(sock, echoBuffer, RCVBUFSIZE-1, 0, (struct sockaddr*)&from_addr, &addr_size)) < 0) {
		DieWithError("recv() failed or connection closed prematurely");
	}
	if(bytesRcvd == 0) {
		printf("Disconnected");
		close(sock);
		exit(0);
	}
	printf("msg<- %s\n",echoBuffer);

	while(1) {
		printf("msg-> ");
		fgets(echoString, RCVBUFSIZE-1, stdin);
		echoString[strlen(echoString)-1] = '\0';
		echoStringLen = strlen(echoString);

		if(strcmp(echoString, "FT") == 0) {
			FTClient(sock, serv_addr);
		} else {
			msgType = EchoReq;
			if(sendto(sock, &msgType, sizeof(char), 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) != sizeof(char)) {
				DieWithError("send() sent a different number of bytes than expected");
			}
			if(sendto(sock, echoString, echoStringLen, 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) != echoStringLen) {
				DieWithError("send() sent a different number of bytes than expected");
			}
			if(strcmp(echoString, "/quit") == 0) {
				break;
			}
			if((bytesRcvd = recvfrom(sock, &msgType, sizeof(char), 0, (struct sockaddr*)&from_addr, &addr_size)) < 0) {
				DieWithError("recv() failed or connection closed prematurely");
			}
			if(msgType != EchoRep) {
				DieWithError("Bad request...");
			}
			if((bytesRcvd = recvfrom(sock, echoBuffer, RCVBUFSIZE-1, 0, (struct sockaddr*)&from_addr, &addr_size)) < 0) {
				DieWithError("recv() failed or connection closed prematurely");
			}
			if(bytesRcvd == 0) {
				printf("Disconnected\n");
				break;
			}
			echoBuffer[bytesRcvd] = '\0';
			printf("msg<- %s\n", echoBuffer);
		}
	}

	close(sock);
	exit(0);
}
