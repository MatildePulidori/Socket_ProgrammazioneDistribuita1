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
#define MAX_LINE 1024
#define MAX_BYTES 4



#define MACOSX 0
#if MACOSX
int flag = SO_NOSIGPIPE;
#else
int flag= MSG_NOSIGNAL;
#endif

void intHandler(int);
int sendn(int sock, uint8_t *ptr, size_t nbytes, int flags);
void sendError(int sock, char *error);
char *prog_name;
uint8_t *answer;

int main (int argc, char *argv[])
{
	int s;
	int sock;
	socklen_t serverAddrLength;
	struct sockaddr_in serverAddr;

	char buffer[MAX_INPUT + MAX_LENGTH_FILENAME];
	int sizeMsgReceived;
	char *ptr;
	char fname[MAX_LENGTH_FILENAME+1]; // +1 = '\0'
	int count = 0;
	char error[] = "-ERR\r\n";

	fd_set cset;
	struct timeval tval;
	int t = 15;

	FILE *file_toFind;
	struct stat infos;
	uint8_t *startptr;
  	//uint8_t *answer, *startptr;
	int tot=0, n =0, sent=0;
	uint8_t sizeByte = 0;
	int j=0, i=0;
	
	signal (SIGINT, intHandler);
	// APERTURA SOCKET
	if ( (s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) <0){
		printf("Error in creating socket \n");
		return -1;
	}
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(atoi(argv[1])); //host to network short per la porta
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); //host to network long per l'indirizzo
	serverAddrLength = sizeof(serverAddr);

	// BIND
	if ( bind (s, (const struct sockaddr * ) &serverAddr, serverAddrLength) <0){
		printf("Error in binding and port. \n");
		return -2;
	}

	// LISTEN
	if ( listen(s, LISTEN_BACKLOG) < 0 ){
		printf("Error in listening. \n");
		return -3;
	}

	answer = (uint8_t *)malloc( BUFF_DIM );
	while (1){
		tot = 0;
		sent = 0;
		
		memset(buffer, 0, MAX_LENGTH_FILENAME+MAX_INPUT);
		
		printf("Waiting a connection ...\n\n");
		// ACCEPT
		if ( (sock = accept(s, (struct sockaddr *) &serverAddr, &serverAddrLength)) <0){
			printf("Connection not accepted already. \n");
			continue;
		}


		while(1){

				// TIMER 15 secondi 
				
				FD_ZERO(&cset);
				tval.tv_sec= t;
				tval.tv_usec= 0;
				FD_SET(sock, &cset);

				if ( (n= select(FD_SETSIZE, &cset, NULL, NULL, &tval) )== -1){
					printf("Error in select()\n");
					break;

				} else if (n==0){
					printf ("Timeout %d secs expired. Try again. \n ", t);
					break;
				}

				// RECEIVING
				sizeMsgReceived = Readline(sock, buffer, (size_t)(MAX_INPUT+MAX_LENGTH_FILENAME));
				if ( sizeMsgReceived <0){  //Errore nella ricezione
					printf("Error in receiving data. \n");
					sendError(sock, error);
					break;
				}
				if (sizeMsgReceived == 0){ //Non c'è più nulla da leggere
					printf("No more data to read: client closed the connection of this socket. \n\n");
					close(sock);
					break;
				}

				// Inizio controlli sulla stringa ricevuta dal client
				// che dovrebbe essere <GET filenameCRLF>


				// 1- controllo GET
				ptr = strstr(buffer, "GET");
				if (ptr == NULL){
					printf("There is no GET command. \n");
					sendError(sock, error);					
					break;

				} else if (ptr != buffer){
					printf("GET command is not in start position. \n");
					sendError(sock, error);
					break;
				}
				ptr = ptr +3;

				// 2- ptr adesso dovrebbe puntare allo spazio,
				// controllo spazio dopo il GET
				if (*ptr != ' '){
					printf("There is no space between GET and filename. \n ");
					sendError(sock, error);
					break;
				}

				count=0;
				ptr++; // ptr dovrebbe puntare al filename 
				for (; (*ptr!='\r' && *(ptr+1)!='\n') && count<(sizeMsgReceived-strlen("GET ")); ptr++, count++);
				
				// 3- Controllo che in fondo ci sia CRLF
				if(count>=(sizeMsgReceived-strlen("GET "))){
					printf("Unable to find CR-LF. \n");
					sendError(sock, error);
					break;
				}

				// 4- Aggiungo il terminatore di stringa
				*(ptr+2)='\0';
				ptr-=count;
				count=0;
				
				// 5- A questo punto mi salvo il nome del file,
				// che poi dovro andare a cercare nella mia directory
				if ( sscanf(ptr, "%s", fname) != 1){ 
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

				// 1 - +OK\r\n
				if (tot + 5*sizeof(char)> BUFF_DIM){ 
					if (sendn(sock, answer, tot, flag)!= tot - 5*sizeof(char)){
						printf("Error in sending answer to the client.\n");
						sendError(sock, error);
						break;
					}
					memset(answer, 0, BUFF_DIM);
					sent+=tot;
					tot=0;
				}

				strcpy((char *)answer, "+OK\r\n"); 
				tot += 5*sizeof(char);
				
				// 2 - +OK\r\nB1.B2.B3.B4
				uint32_t sizeFileReceived = infos.st_size; // file dimension, host byte order
				if (sizeFileReceived> 0xFFFFFFFF){
					printf("File dimension does not fit 32bit. \n");
					sendError(sock, error);
					break;
				}
				if (tot+sizeof(uint32_t)>BUFF_DIM){ 
					if (sendn(sock, answer, tot, flag)!= tot-sizeof(uint32_t)){ 
						printf("Error in sending answer to client.\n");
						sendError(sock, error);
						break;
					}

					memset(answer, 0, BUFF_DIM);
					sent+=tot;
					tot=0;
				}

				printf("File dimension: %u bytes\n", sizeFileReceived);
				startptr = answer; 

				
				j = 5;
				int offset=24;
				uint32_t mask  = 0xFF000000;

				for(i = 0; i < 4; i++) {
							sizeByte = (sizeFileReceived & mask)>>offset ;
							answer[j] = sizeByte;
							j++;
							offset -=8;
							mask >>= 8;
				}
				tot += sizeof(uint32_t);
				if ((tot+MAX_LINE)>=BUFF_DIM){    
					if(sendn(sock, answer, tot, flag)!=tot){
						printf("Error in sending answer to client.\n");
						sendError(sock, error);
						break;
					}
					memset(answer, 0, BUFF_DIM);
					sent+=tot;
					tot=0;
				}



				answer += tot; // answer come puntatore alla stringa dopo OK\r\nBYTES

				// 3 -  +OK\r\nB1.B2.B3.B4.data
				while( (n = fread(answer, sizeof(char), MAX_LINE, file_toFind) ) > 0 ) {
					tot+=n;
					if ((tot+MAX_LINE)>=BUFF_DIM){
						answer = startptr;
						if (sendn(sock, answer, tot, flag)!= tot){
							printf("Error in sending answer to client.\n");
							sendError(sock, error);
							break;
						}
						memset(answer, 0, BUFF_DIM);
						sent+=tot;
						tot=0;
					}else{
						answer += n;
					}
				}
				answer = startptr;


				// 4 - +OK\r\nB1.B2.B3.B4.data.T1.T2.T3.T4
				uint32_t timestampFileToSend = infos.st_ctime;
				if (timestampFileToSend> 0xFFFFFFFF){
						printf("File timestamp does not fit 32bit. \n");
						sendError(sock, error);
						break;
				}
				if(tot +sizeof(uint32_t)>= BUFF_DIM){
					if (sendn(sock, answer, tot, flag)!=tot-sizeof(uint32_t)){
						printf("Error in sending answer to client.\n");
						sendError(sock, error);
						break;
					}
					memset(answer, 0, BUFF_DIM);
					sent+=tot-sizeof(uint32_t);
					tot=0;
				}else{
						
						mask = 0xFF000000;
						offset=24;
						j=tot;

						for(i=0; i<4; i++){
							answer[j]=((timestampFileToSend & mask)>> offset);
							j++;
							mask >>=8;
							offset -=8;
						}
						tot += sizeof(uint32_t);

						if (sendn(sock, answer, tot, flag)!= tot){
								printf("Error sending answer to the client.\n" );
								sendError(sock, error);
								break;
						}
						memset(answer, 0, BUFF_DIM);
						sent+=tot;
						tot=0;
				}
				fclose(file_toFind);
				printf("File '%s' sent all.\n\n", fname);
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





