#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void DieWithError(char *errorMessage);
void HandleUDPClient(int clntSocket);

int main(int argc, char *argv[]) {
	int serv_sock;

	struct sockaddr_in serv_addr;

	if((serv_sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		DieWithError("socket() failed");
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(30405);

	if(bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
		DieWithError("bind() failed");
	}

	while(1) {
		HandleUDPClient(serv_sock);
	}

	close(serv_sock);
	return 0;
}
