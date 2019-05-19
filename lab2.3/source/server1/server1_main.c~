/*
 * TEMPLATE
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "../sockwrap.h"

#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <signal.h>

#define LISTEN_BACKLOG 15
#define MAX_LENGTH_FILENAME 255
#define MAX_INPUT 6
#define LENGTH_ERROR 6
#define BUFF_DIM 4096
#define LINE 1024
#define MAX_BYTES 4




int flag= MSG_NOSIGNAL;
void intHandler(int);
int sendn(int sock, uint8_t *ptr, size_t nbytes, int flags);
int checkCommandReceived( char *msg, int size);
int getFileName(char *msg, int size, char filename[]);
int writeDimension(uint32_t sizeFile, uint32_t *sizePointer );
int writeTimestamp(uint32_t tsFile, uint32_t *tsPointer);

void sendError(int sock, char *error);
char *prog_name;
uint8_t *answer;
char error[] = "-ERR\r\n";

int main (int argc, char *argv[])
{
	int s, sock;
	socklen_t saddrLength;
	struct sockaddr_in sAddr;

	char b[MAX_INPUT + MAX_LENGTH_FILENAME];
	int sizeMsgReceived;
	char fname[MAX_LENGTH_FILENAME+1]; // +1 = '\0'
	
	fd_set cset;
	struct timeval tval;
	int t = 15;

	FILE *file_toFind;
	struct stat infos;
	uint8_t  *startPointer;
	int tot=0, n=0;
	
	signal (SIGINT, intHandler);
	// APERTURA SOCKET
	if ( (s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) <0){
		printf("Error in creating socket \n");
		return -1;
	}
	sAddr.sin_family = AF_INET;
	sAddr.sin_port = htons(atoi(argv[1])); //host to network short 
	sAddr.sin_addr.s_addr = htonl(INADDR_ANY); //host to network long
	saddrLength = sizeof(sAddr);

	// BIND
	if ( bind (s, (const struct sockaddr * ) &sAddr, saddrLength) <0){
		printf("Error in binding and port. \n");
		return -2;
	}

	// LISTEN
	if ( listen(s, LISTEN_BACKLOG) < 0 ){
		printf("Error in listening. \n");
		return -3;
	}

	answer = (uint8_t *)malloc( BUFF_DIM );
	for(;;){
		tot = 0;
		memset(b, 0, MAX_LENGTH_FILENAME+MAX_INPUT);
		printf("Waiting connection..\n");
		// ACCEPT
		if ( (sock = accept(s, (struct sockaddr *) &sAddr, &saddrLength)) <0){
			printf("Connection not accepted already. \n");
			continue;
		}
		


		for(;;){

				// TIMER 15 secondi 
				FD_ZERO(&cset);
				tval.tv_sec= t;
				tval.tv_usec= 0;
				FD_SET(sock, &cset);

				if ( (n= select(FD_SETSIZE, &cset, NULL, NULL, &tval) )== -1){
					printf("Error in select\n");
					sendError(sock, error);
					break;

				} else if (n==0){
					printf ("Timeout %d secs expired. Try again. \n ", t);
					sendError(sock, error);
					break;
				}
				

				// RECEIVING
				sizeMsgReceived = Readline(sock, b, (size_t)(MAX_INPUT+MAX_LENGTH_FILENAME));
				if ( sizeMsgReceived <0){  
					sendError(sock, error);
					break;
				}
				if (sizeMsgReceived == 0){ 
					printf("No more data.\n");
					close(sock);
					break;
				}
				
				
				char *cmdPointer = b;
				cmdPointer += sizeMsgReceived;
				*cmdPointer = '\0';
				cmdPointer -= sizeMsgReceived;

				// Inizio controlli sulla stringa ricevuta dal client
				// che dovrebbe essere <GET filenameCRLF>
		
				if (checkCommandReceived(cmdPointer, sizeMsgReceived)!=sizeMsgReceived){
					sendError(sock, error);
					break;
				}
				cmdPointer += strlen("GET ");

				// 5- Mem file name
				if ( getFileName(cmdPointer, (sizeMsgReceived-MAX_INPUT), fname) != 0){ 
					printf ("Error in filename. \n");
					sendError(sock, error);
					break;
				}
				
				// In fname there is the name of the file the server have to send to the client
				// printf("File name is: '%s'\n", fname);

				// Apriamo il file in lettura
				file_toFind = fopen(fname, "rb");
				if (file_toFind==NULL){
					printf("File %s does not exists\n", fname );
					sendError(sock, error);
					break;
				}

				// Informations about the file
				if (stat(fname,  &infos) == -1){
					printf("Error in getting informations about file %s \n", fname );
					sendError(sock, error);
					fclose(file_toFind);
					break;
				}

				// ANSWER TO THE CLIENT
				tot=0;
				memset(answer, 0, BUFF_DIM);

				// 1 - +OK\r\n
				

				if ( tot + 5 >= BUFF_DIM){
					if(sendn(sock, answer, tot, flag)!=tot){
						sendError(sock, error);
						break;
					}
					tot=0;
					memset(answer, 0, BUFF_DIM);
				}

				strcpy((char *)answer, "+OK\r\n"); 
				tot += 5;
		

				// 2 - +OK\r\nB1.B2.B3.B4
				uint32_t sizeFileReceived = infos.st_size; // file dimension, host byte order
				if (sizeFileReceived> 0xFFFFFFFF){
					printf("File dimension does not fit 32bit. \n");
					sendError(sock, error);
					break;
				}
				
				if ( tot + sizeof(uint32_t) >= BUFF_DIM){
					if(sendn(sock, answer, tot, flag)!=tot){
						sendError(sock, error);
						break;
					}
					tot=0;
					memset(answer, 0, BUFF_DIM);
				}
				startPointer = answer;
				answer = answer+tot; 
				sizeFileReceived = htonl(sizeFileReceived);
				writeDimension(sizeFileReceived, (uint32_t *)answer);
				answer = startPointer;
				tot += sizeof(uint32_t);

				if ( tot + LINE > BUFF_DIM){
					if(sendn(sock, answer, tot, flag)!=tot){
						sendError(sock, error);
						break;
					}
					tot=0;
					memset(answer, 0, BUFF_DIM);
				}
				answer += tot; // answer come puntatore alla stringa dopo OK\r\nBYTES

				// 3 -  +OK\r\nB1.B2.B3.B4.data
				while( (n = fread(answer, sizeof(char), LINE, file_toFind) ) > 0 ) {
					tot+=n;
					if ((tot+n)>=BUFF_DIM){
						answer = startPointer;
						if (sendn(sock, answer, tot, flag)!= tot){
							sendError(sock, error);
							break;
						}
						memset(answer, 0, BUFF_DIM);
						tot=0;
					}else{
						answer += n;
					}
				}
			

				// 4 - +OK\r\nB1.B2.B3.B4.data.T1.T2.T3.T4
				uint32_t timestampFileToSend = infos.st_ctime;
				if (timestampFileToSend> 0xFFFFFFFF){
						printf("File timestamp does not fit 32bit. \n");
						sendError(sock, error);
						break;
				}
				if(tot +sizeof(uint32_t)>=  BUFF_DIM){
					answer=startPointer;
					if (sendn(sock, answer, tot, flag)!=tot){
						sendError(sock, error);
						break;
					}
					printf("ciao!\n");
					memset(answer, 0, BUFF_DIM);
					tot=0;
				}else{
					timestampFileToSend = htonl(timestampFileToSend);
					writeTimestamp(timestampFileToSend, (uint32_t * )answer);
					answer=startPointer;
					tot+=sizeof(uint32_t);
					
					if (sendn(sock, answer, tot, flag)!= tot){
							sendError(sock, error);
							break;
					}
					
					memset(answer, 0, BUFF_DIM);
					tot=0;
				}
				fclose(file_toFind);
				//printf("File '%s' sent all.\n\n", fname);
		}
		
	}
	free(answer);
	
	return 0;
}



// ---------------------------------------------------------------------------

int sendn(int sock, uint8_t *ptr, size_t nbytes, int flags){
	size_t nleft;
	ssize_t nwritten;

    for (nleft=nbytes; nleft > 0; ) {
    	nwritten = send(sock, ptr, nleft, flags);
		if (nwritten <=0)
		 	return (nwritten);
        else {
            nleft -= nwritten;
            ptr += nwritten;
		}
	}

    return (nbytes - nleft);
}


int checkCommandReceived(char *msg, int size){
	int i=0, j=0;
	char get[4];
	char filename[size-strlen("GET ")+1];
	
	for(i=0, j=0; i<3; i++, j++, msg++){
		get[i]=(*msg);
	}
	get[i]='\0';
	int res = strcmp(get, "GET");
	if (res!=0){
		return -1;		
	}
	if(*msg != ' '){ 
		return -1;
	}
	j++;
	msg++; // j=4
	for (i=0; i< (size-strlen("GET ")); i++, j++, msg++){
		if(*msg!='\r'){
			filename[i]=*msg;
		}
		else{
			break;
		}
	}
	filename[i] ='\0';	
	if (*msg != '\r'){
		return -1;
	}
	msg++;
	j++;
	if (*msg!='\n'){
		return -1;
	}
	j++;
	if (j> size){
		return -1;
	}
	return j; 
	
}

int getFileName(char *msg, int size, char filename[]){
	int i=0;
	for (i=0; i< (size); i++, msg++){
		if(*msg!='\r'){
			filename[i]=*msg;
		}
		else{
			break;
		}
	}
	filename[i]='\0';
	if (i>size){
		return -1;
	}
	return 0;
}

int writeDimension(uint32_t sizeF, uint32_t *sizePointer){
	memcpy(sizePointer, &sizeF, sizeof(sizeF));
	return 0;
}

int writeTimestamp(uint32_t tsFile, uint32_t *tsPointer){
	memcpy(tsPointer, &tsFile, sizeof(tsFile));
	return 0;
}

void sendError(int sock, char *error){

	if (send(sock, error, strlen(error), 0)!= strlen(error)){ 
		printf("Error in sending %s\n", error );	
	}
	close(sock);
	
}



void intHandler(int sign){
	free(answer);
	exit(0);
}





