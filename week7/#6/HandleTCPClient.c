#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

#define RCVBUFSIZE	1024

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

struct linkedList {
	struct linkedList* next;
	char *filename;
};

struct linked_list {
	int *clntSock;
	int *ftSock;
	struct linked_list* next;
	struct linked_list* prev;
};

void DieWithError(char *errorMessage);

void HandleTCPClient(struct linked_list *list) {
	int clntSocket, ftSock;
	char echoBuffer[RCVBUFSIZE];
	int recvMsgSize;
	char msgType = 0;

	struct tm *t;
	time_t timer;

	FILE *f;
	FILE *log;
	char filename[FILENAME_LEN];
	long file_size = 0;
	long totalByteRcvd = 0, totalByteSent = 0;
	char str_filesize[32];
	char buf[PACKET_SIZE];
	int fsize=0;
	int percent = 0;
	int i=0;

	struct linkedList *l = NULL;
	struct linkedList *temp = NULL;
	int list_count = 0;

	struct linked_list *tmp;
	clntSocket = *(list->clntSock);
	ftSock = *(list->ftSock);
	while(1) {
		if((recvMsgSize = recv(clntSocket, &msgType, sizeof(char), 0)) < 0) {
			DieWithError("recv() failed");
		}
		if(recvMsgSize == 0) {
			printf("Disconnected\n");
			break;
		}
		if(msgType == EchoReq) {
			// msg rcv & echo
			if((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE-1, 0)) < 0) {
				DieWithError("recv() failed");
			}
			echoBuffer[recvMsgSize] = '\0';
			if(strcmp(echoBuffer, "/quit") == 0 || recvMsgSize == 0) {
				printf("Disconnected\n");
				break;
			}

			timer = time(NULL);
			t = localtime(&timer);

			log = fopen("log.txt","a");
			fprintf(log, "%4d.%2d.%2d. %2d:%2d:%2d MSG: %s\n",t->tm_year+1900,t->tm_mon+1,t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec,echoBuffer);
			fclose(log);

			msgType = EchoRep;
			tmp = list->next;
			while(tmp != list) {
				if(send(*(tmp->clntSock), &msgType, sizeof(char), 0) != sizeof(char)) {
					DieWithError("send() sent a different number of bytes than expected");
				}
				if(send(*(tmp->clntSock), echoBuffer, strlen(echoBuffer), 0) != strlen(echoBuffer)) {
					DieWithError("send() sent a different number of bytes than expected");
				}
				tmp = tmp->next;
			}
		} else if(msgType == FileUpReq) {
			if((recvMsgSize = recv(ftSock, filename, FILENAME_LEN-1, 0)) < 0) {
				DieWithError("recv() failed");
			}
			filename[recvMsgSize] = '\0';
			if((recvMsgSize = recv(ftSock, str_filesize, 31, 0)) < 0) {
				DieWithError("recv() failed");
			}
			str_filesize[recvMsgSize] = '\0';
			file_size = atol(str_filesize);
			f = fopen(filename, "wb");
			if(f == NULL) {
				fclose(f);
				DieWithError("File Open Error");
			} 

			fprintf(stderr, "File receiving <= ");
			//printf("File receiving <= ");

			totalByteRcvd = 0;
			percent = 0;

			while(totalByteRcvd < file_size) {
				if((recvMsgSize = recv(ftSock, buf, PACKET_SIZE, 0)) < 0) {
					DieWithError("recv() failed");
				}
				fwrite(buf, sizeof(char), recvMsgSize, f);
				totalByteRcvd += recvMsgSize;
				for(i=(percent+9)/5; i<((totalByteRcvd*100/file_size)+9)/5; i++) {
					fprintf(stderr, "#");
				}
				percent = totalByteRcvd*100/file_size;
			}
			printf("\n");

			msgType = FileAck;
			if(send(ftSock, &msgType, sizeof(char), 0) != sizeof(char)) {
				DieWithError("send() sent a different number of bytes than expected");
			}

			printf("%s (%ld bytes) downloading success from client\n", filename, file_size);

			fclose(f);

			timer = time(NULL);
 			t = localtime(&timer);
 
 			log = fopen("log.txt","a");
			fprintf(log, "%4d.%2d.%2d. %2d:%2d:%2d FILE_DOWN: %s\n",t->tm_year+1900,t->tm_mon+1,t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec,filename);
 			fclose(log);

		} else if(msgType == FileDownReq) {
			if((recvMsgSize = recv(ftSock, filename, FILENAME_LEN-1, 0)) < 0) {
				DieWithError("recv() failed");
			}
			filename[recvMsgSize] = '\0';

			f = fopen(filename, "rb");
			if(f == NULL) {
				fclose(f);

				msgType = FileNak;
				if(send(ftSock, &msgType, sizeof(char), 0) != sizeof(char)) {
					DieWithError("send() sent a different number of bytes than expected");
				}
			} else {
				msgType = FileAck;
				if(send(ftSock, &msgType, sizeof(char), 0) != sizeof(char)) {
					DieWithError("send() sent a different number of bytes than expected");
				}

				fseek(f, 0, SEEK_END);
				file_size = ftell(f);
				fseek(f, 0, SEEK_SET);

				snprintf(str_filesize, 32, "%ld", file_size);

				if(send(ftSock, str_filesize, 31, 0) != 31) {
					DieWithError("send() sent a different number of bytes than expected");
				}

				totalByteSent = 0;
				percent = 0;

				fprintf(stderr, "File sending (%s) => ", filename);

				while(totalByteSent < file_size) {
					fsize = fread(buf, sizeof(char), PACKET_SIZE, f);
					totalByteSent += fsize;
					if(send(ftSock, buf, fsize, 0) != fsize) {
						DieWithError("send() sent a different number of bytes than expected");
					}
					for(i=(percent+9)/5; i<((totalByteSent*100/file_size)+9)/5; i++) {
						fprintf(stderr, "#");
					}
					percent = totalByteSent*100/file_size;
				}
				printf("\n");

				if((recvMsgSize = recv(ftSock, &msgType, sizeof(char), 0)) < 0) {
					DieWithError("recv() failed");
				}

				printf("%s (%ld bytes) uploading success to client\n", filename, file_size);

				fclose(f);

				timer = time(NULL);
				t = localtime(&timer);

				log = fopen("log.txt","a");
				fprintf(log, "%4d.%2d.%2d. %2d:%2d:%2d FILE_UP\n: %s",t->tm_year+1900,t->tm_mon+1,t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec,filename);
				fclose(log);
			}
		} else if(msgType == LSReq) {
			system("ls > .ls.log");

			f = fopen(".ls.log", "r");

			if(f == NULL) {
				fclose(f);
				
				msgType = LSNak;
				if(send(ftSock, &msgType, sizeof(char), 0) != sizeof(char)) {
					DieWithError("send() sent a different number of bytes than expected");
				}
			} else {
				msgType = LSAck;
				if(send(ftSock, &msgType, sizeof(char), 0) != sizeof(char)) {
					DieWithError("send() sent a different number of bytes than expected");
				}

				list_count = 0;

				while(!feof(f)) {
					fgets(filename, FILENAME_LEN-1, f);
					filename[strlen(filename)-1] = '\0';
					if(strlen(filename) != 0 && filename[0] != '\0' && filename[0] != '\n') {
						if(list_count == 0) {
							l = (struct linkedList*)malloc(sizeof(struct linkedList));
							temp = l;
						} else {
							temp->next = (struct linkedList*)malloc(sizeof(struct linkedList));
							temp = temp->next;
						}

						temp->filename = (char*)malloc(strlen(filename));
						strcpy(temp->filename, filename);
						list_count++;
					}
				}

				list_count--;

				fclose(f);
				system("rm .ls.log");

				snprintf(str_filesize, 32, "%d", list_count);

				if(send(ftSock, str_filesize, 31, 0) != 31) {
					DieWithError("send() sent a different number of bytes than expected");
				}

				temp = l;
				for(i=0; i<list_count; i++) {
					if(send(ftSock, temp->filename, FILENAME_LEN-1, 0) != FILENAME_LEN-1) {
						DieWithError("send() sent a different number of bytes than expected");
					}
					l = temp;
					temp = temp->next;
					free(l);
				}
			}
		} else {
			DieWithError("Bad request...");
		}
	}

	close(clntSocket);
	close(ftSock);
}
