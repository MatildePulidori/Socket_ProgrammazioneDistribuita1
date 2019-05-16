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

#define LISTEN_BACKLOG 15
#define MAX_LENGTH_FILENAME 255
#define MAX_INPUT 6
#define LENGTH_ERROR 6
#define BUFF_DIM 4096
#define MAX_LINE 1024
#define MAX_BYTES 4



#define MACOSX 1
#if MACOSX
int flag = SO_NOSIGPIPE;
#else
int flag= MSG_NOSIGNAL;
#endif


int sendn(int sock, uint8_t *ptr, size_t nbytes, int flags);
char *prog_name;

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

	//fd_set cset;
	struct timeval tval;
	int t = 15;

	FILE *file_toFind;
	struct stat infos;
  	uint8_t *answer, *startptr;
	int tot=0, n =0, sent=0;
	uint8_t sizeByte = 0;
	int j=0, i=0;


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
		pid_t childpid;
		serverAddrLength = sizeof(serverAddr);
		memset(buffer, 0, MAX_LENGTH_FILENAME+MAX_INPUT);
		memset(answer, 0, BUFF_DIM);
		printf("\nWaiting a connection ...\n\n");

		// ACCEPT
		if ( (sock = accept(s, (struct sockaddr *) &serverAddr, &serverAddrLength)) <0){
			printf("Connection accept failed. \n");
			continue;
		}
		
		if ((childpid=fork())<0){
			printf("Fork failed.\n");
			continue;
		}else{
			if(childpid>0){
			close(sock);
		} else{
			
			while(1){
					
					// TIMER 15 secondi 
					fd_set cset;
					FD_ZERO(&cset);
					tval.tv_sec= t;
					tval.tv_usec= 0;
					FD_SET(sock, &cset);
					
					if ( (n= select(FD_SETSIZE, &cset, NULL, NULL, &tval) )== -1){
						printf("Errore in select()\n");
						break;

					} else if (n==0){
						printf ("Timeout %d secs expired. Try again. \n ", t);
						break;
					}
			
					// RECEIVING
					sizeMsgReceived = Readline(sock, buffer, (size_t)MAX_INPUT+MAX_LENGTH_FILENAME);
					
					if ( sizeMsgReceived <0){  //Errore nella ricezione
						printf("Error in receiving data. \n");
						if (send(sock, error, sizeof(error), 0)!= sizeof(error)){ // vedi sopra
							printf("Error in sending %s\n", error );
							break;
						}
						break;
					}
					if (sizeMsgReceived == 0){ //Non c'è più nulla da leggere
						printf("No more data to read and client closed the connection of this socket. \n\n");
						close(sock);
						break;
					}

					// Inizio controlli sulla stringa ricevuta dal client
					// che dovrebbe essere <GET filenameCRLF>


					// 1- controllo GET
					ptr = strstr(buffer, "GET");
					if (ptr == NULL){
						printf("Missing GET command. \n");
						// TO DO -> -ERRCRLF
						if (send(sock, error, sizeof(error), 0)!= sizeof(error)){
							// con sizeof() non funziona perche' prendi la size del puntatore (4 byte)
							// invece devi mettere strlen(error) sia prima che dopo il !=
							printf("Error in sending %s\n", error );
							break;
						}
						break;

					} else if (ptr != buffer){
						printf("GET command is not in the start position. \n");
						if (send(sock, error, sizeof(error), 0)!= sizeof(error)){ // vedi sopra
							printf("Error in sending %s\n", error );
							break;
						}
						break;
					}
					ptr = ptr +3;

					// 2- ptr adesso dovrebbe puntare allo spazio,
					// controllo spazio dopo il GET
					if (*ptr != ' '){
						printf("Missing space between GET and filename. \n ");
						if (send(sock, error, sizeof(error), 0)!= sizeof(error)){
							printf("Error in sending %s\n", error );
							break;
						}
						break;
					}

					
					ptr++; // ptr dovrebbe puntare al filename 
					for (; (*ptr!='\r' && *(ptr+1)!='\n') && count<sizeMsgReceived; ptr++, count++);


					// 3- Controllo che in fondo ci sia CRLF
					if(count>sizeMsgReceived){
						printf("Unable to find CR-LF. \n");
						if ( send(sock, error, sizeof(error), 0) != sizeof(error)){ // vedi sopra
							printf("Error in sending %s\n", error );
							break;

						}

						break;
					}

					// 4- Aggiungo il terminatore di stringa
					*(ptr+2)='\0';
					ptr-=count;
					count=0;
					
					// 5- A questo punto mi salvo il nome del file,
					// che poi dovro andare a cercare nella mia directory
					if ( sscanf(ptr, "%s ", fname) != 1){ // ma questo spazio dopo %s??
						printf ("Error in filename. \n");

						if (send(sock, error, sizeof(error), 0)!= sizeof(error)){
							printf("Error in sending %s\n", error );
							break;
						}
						break;
					}

					// In fname there is the name of the file the server have to send to the client
					printf("File name is: '%s'\n", fname);
					// Apriamo il file in lettura
					file_toFind = fopen(fname, "rb");
					if (file_toFind==NULL){
						printf("File %s does not exists\n", fname );
						if (send(sock, error, sizeof(error), 0)!= sizeof(error)){
							printf("Error in sending %s\n", error );
							break;
						}
						break;
					}

					// Informations about the file
					if (stat(fname,  &infos) == -1){
						printf("Error in getting informations about file %s \n", fname );
						perror("");
						fclose(file_toFind);
						break;
					}

					//ANSWER TO THE CLIENT
					if (tot + 5*sizeof(char)> BUFF_DIM){ // se +OK\r\n non entra ... errore e poi svuoto buffer answer

						if (sendn(sock, answer, tot, flag)!= tot - 5*sizeof(char)){
							printf("Error in sending response to the client.\n");
							break;
						}
						memset(answer, 0, BUFF_DIM);
						sent+=tot;
						tot=0;
					}

					strcpy((char *)answer, "+OK\r\n");   // scrivo su buffer answer +OK\r\n
					tot += 5*sizeof(char);

					uint32_t sizeFileReceived = infos.st_size; // file dimension, in bytes
					if (sizeFileReceived> 0xFFFFFFFF){
							printf("File dimension does not fit 32bit. \n");
							break;
					}
					if (tot+sizeof(uint32_t)>BUFF_DIM){ // se la dimensione del file BYTES non sta nel buffer answer ..
						if (sendn(sock, answer, tot, flag)!= tot-sizeof(uint32_t)){ // errore e poi svuoto il buffer answer
							printf("Error in sending answer to client.\n");
							break;
						}

						memset(answer, 0, BUFF_DIM);
						sent+=tot;
						tot=0;
						j=0;
					}

					// scrivo sul buffer answer (+OK\r\n)BYTES
					printf("File %s dimension: %u bytes\n", fname, sizeFileReceived);
					startptr = answer; // puntatore al blocco di memoria dove viene salvata la risposta, prima di +OK\r\n

					j = 5;
					int offset=24;
					uint32_t mask  = 0xFF000000;

					for(i = 0; i < 4; i++, mask >>= 8, offset -= 8, j++) {
								sizeByte = (sizeFileReceived & mask)>>offset ;
								answer[j] = sizeByte;
					};
					tot += sizeof(uint32_t);
					if ((tot+MAX_LINE)>=BUFF_DIM){      // invio answer +OK\r\nBYTES
						if(sendn(sock, answer, tot, flag)!=tot){
							printf("Error in sending answer to client.\n");
						}
						memset(answer, 0, BUFF_DIM);
						sent+=tot;
						tot=0;
					}


					answer += tot; // metto answer come puntatore alla stringa dopo OK\r\nBYTES
					//printf("DOCUMENTO: \n");

					while( (n = fread(answer, sizeof(char), MAX_LINE, file_toFind) ) > 0 ) {
						tot+=n;
						if ((tot+MAX_LINE)>=BUFF_DIM){
							answer = startptr;
							if (sendn(sock, answer, tot, flag)!= tot){
								printf("Error in sending answer to client.\n");
								break;
							}
							sent+=tot;
							tot=0;
							memset(answer, 0, BUFF_DIM);

						}else{
							answer += n;
						}
					}
					
					answer = startptr;
					uint32_t timestampFileToSend = infos.st_ctime;
					if (timestampFileToSend> 0xFFFFFFFF){
							printf("File timestamp does not fit 32bit. \n");
							if (sendn(sock, answer, tot, flag)!=tot-sizeof(uint32_t)){
								printf("Error in sending answer to client.\n");
								break;
							}
							break;
					}
					if(tot +sizeof(uint32_t)>= BUFF_DIM){
						if (sendn(sock, answer, tot, flag)!=tot-sizeof(uint32_t)){
							printf("Error in sending answer to client.\n");
							break;
						}
						memset(answer, 0, BUFF_DIM);
						sent+=tot-sizeof(uint32_t);
						tot=0;
					}else{

							mask = 0xFF000000;
							offset=24;
							j=tot;
							for(i=0; i<4; mask>>=8, offset-=8, j++, i++){
								answer[j]=((timestampFileToSend & mask)>> offset);
							}
							tot += sizeof(uint32_t);
							if (sendn(sock, answer, tot, flag)!= tot){
									printf("Error sending data.\n" );
									break;
							}
							memset(answer, 0, BUFF_DIM);
							sent+=tot;
							tot=0;
					}
					fclose(file_toFind);
					printf("\nFile '%s' sent all.\n\n", fname);

			}
			exit(0);
		}
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

