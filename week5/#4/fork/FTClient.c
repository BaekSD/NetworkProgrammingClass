#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define EchoReq	01
#define FileUpReq	02
#define FileDownReq	03
#define LSReq	04
#define EchoRep	11
#define FileAck	12
#define FileNak	13
#define LSAck	14
#define LSNak	15

#define FILENAME_LEN	256
#define PACKET_SIZE	256

void DieWithError(char *errorMessage);

void FTClient(int sock) {
	char input = 0;
	char filename[FILENAME_LEN];
	long file_size = 0;
	char str_filesize[32];
	char msgType = 0;
	long totalByteSent = 0;
	long totalByteRcvd = 0;
	char buf[PACKET_SIZE];
	int fsize=0;
	FILE *f;
	int percent = 0;
	int i=0;
	int bytesRcvd = 0;

	printf("Welcome to Socket FT Client\n");

	while(1) {
		printf("ftp command [p)ut\tg)et\tl)s\tr)ls\te)xit ] -> ");
		input = getchar();
		//fgets(&input, sizeof(char), stdin);
		while(getchar() != '\n');

		if(input == 'p') {
			printf("filename to put to server -> ");
			fgets(filename, FILENAME_LEN-1, stdin);
			filename[strlen(filename)-1] = '\0';

			f = fopen(filename, "rb");
			if(f == NULL) {
				printf("FileNotFoundException\n");
				continue;
			}

			fseek(f, 0, SEEK_END);
			file_size = ftell(f);
			fseek(f, 0, SEEK_SET);

			if(file_size <= 0) {
				printf("FileSizeZeroException\n");
				continue;
			}

			snprintf(str_filesize, 32, "%ld", file_size);

			msgType = FileUpReq;
			if(send(sock, &msgType, sizeof(char), 0) != sizeof(char)) {
				DieWithError("send() sent a different number of bytes than expected");
			}
			if(send(sock, filename, FILENAME_LEN-1, 0) != FILENAME_LEN-1) {
				DieWithError("send() sent a different number of bytes than expected");
			}
			if(send(sock, str_filesize, 31, 0) != 31) {
				DieWithError("send() sent a different number of bytes than expected");
			}

			totalByteSent = 0;
			percent = 0;

			fprintf(stderr, "sending => ");
			//printf("sending => ");

			while(totalByteSent < file_size) {
				fsize = fread(buf, sizeof(char), PACKET_SIZE, f);
				totalByteSent += fsize;
				if(send(sock, buf, fsize, 0) != fsize) {
					DieWithError("send() sent a different number of bytes than expected");
				}
				for(i=(percent+9)/5; i<((totalByteSent*100/file_size)+9)/5; i++) {
					fprintf(stderr, "#");
					//printf("#");
				}
				percent = totalByteSent*100/file_size;
			}
			printf("\n");

			if((bytesRcvd = recv(sock, &msgType, sizeof(char), 0)) < 0) {
				DieWithError("recv() failed or connection closed prematurely");
			}

			printf("%s (%ld bytes) uploading success to server\n", filename, file_size);

			fclose(f);
		} else if(input == 'g') {
			printf("filename to get from server -> ");
			fgets(filename, FILENAME_LEN-1, stdin);
			filename[strlen(filename)-1] = '\0';

			msgType = FileDownReq;

			if(send(sock, &msgType, sizeof(char), 0) != sizeof(char)) {
				DieWithError("send() sent a different number of bytes than expected");
			}
			if(send(sock, filename, FILENAME_LEN-1, 0) != FILENAME_LEN-1) {
				DieWithError("send() sent a different nubmer of bytes than expected");
			}

			if((bytesRcvd = recv(sock, &msgType, sizeof(char), 0)) < 0) {
				DieWithError("recv() failed or connection closed prematurely");
			}

			if(msgType == FileNak) {
				printf("No such File : %s\n",filename);
			} else if(msgType == FileAck) {
				if((bytesRcvd = recv(sock, str_filesize, 31, 0)) < 0) {
					DieWithError("recv() failed or connection closed prematurely");
				}
				str_filesize[bytesRcvd] = '\0';
				file_size = atol(str_filesize);
				f = fopen(filename, "wb");
				if(f == NULL) {
					fclose(f);
					DieWithError("File Open Error");
				}

				fprintf(stderr, "Receiving <= ");

				totalByteRcvd = 0;
				percent = 0;

				while(totalByteRcvd < file_size) {
					if((bytesRcvd = recv(sock, buf, PACKET_SIZE, 0)) < 0) {
						DieWithError("recv() failed or connection closed prematurely");
					}
					fwrite(buf, sizeof(char), bytesRcvd, f);
					totalByteRcvd += bytesRcvd;
					for(i=(percent+9)/5; i<((totalByteRcvd*100/file_size)+9)/5; i++) {
						fprintf(stderr, "#");
					}
					percent = totalByteRcvd*100/file_size;
				}
				printf("\n");

				msgType = FileAck;
				if(send(sock, &msgType, sizeof(char), 0) != sizeof(char)) {
					DieWithError("send() sent a different number of bytes than expected");
				}

				printf("%s (%ld bytes) downloading success from server\n", filename, file_size);

				fclose(f);
			}

		} else if(input == 'l') {
			system("ls");
		} else if(input == 'r') {
			msgType = LSReq;
			if(send(sock, &msgType, sizeof(char), 0) != sizeof(char)) {
				DieWithError("send() sent a different number of bytes than expected");
			}

			if((bytesRcvd = recv(sock, &msgType, sizeof(char), 0)) != sizeof(char)) {
				DieWithError("recv() failed or connection closed prematurely");
			}

			if(msgType == LSNak) {
				printf("Can't load file list of server\n");
			} else if(msgType == LSAck) {
				if((bytesRcvd = recv(sock, str_filesize, 31, 0)) < 0) {
					DieWithError("recv() failed or connection closed prematurely");
				}
				str_filesize[bytesRcvd] = '\0';
				file_size = atol(str_filesize);

				for(i=0; i<file_size; i++) {
					if((bytesRcvd = recv(sock, filename, FILENAME_LEN-1, 0)) < 0) {
						DieWithError("recv() failed or connection closed prematurely");
					}
					filename[bytesRcvd] = '\0';
					printf("%s\n",filename);
				}
			}
		} else if(input == 'e') {
			return;
		} else {
			printf("incorrect input...\n");
		}
	}
}
