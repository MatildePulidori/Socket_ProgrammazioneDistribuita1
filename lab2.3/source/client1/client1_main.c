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
#include <signal.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <string.h>
#include <time.h>
#include <unistd.h>

#define BUFF_DIM 4096

char *prog_name;
char *bufferToSend;

int main (int argc, char *argv[])
{
	struct sockaddr_in saddr;
	int s;
	int result;
	uint8_t answer[BUFF_DIM];
	

	int k=3, i=0, j=0, n=0, t=15;
	char error[] = "-ERR\r\n";
	fd_set cset;
	struct timeval tval;

	FILE *file_toWrite;
	int bytes_rec = 0, tot=0, end=0, toRead=0, written=0;
	uint32_t fileDimension=0, fileLastModified=0;


	if (argc < 4){
		printf("Error in parameters.\n");
		return -1;
	}

	s = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
	if( s < 0 )
	{
		printf("Error in creating socket.\n");
		return -1;
	}

	// Socke info
	saddr.sin_family = AF_INET;
	if( inet_aton(argv[1],&(saddr.sin_addr)) == 0 ){
		printf("Error in inet_aton() for address %s.\n", argv[1]);
		return -2;
	}
	saddr.sin_port = htons( atoi(argv[2]) );

	// Connect
	result = connect( s, (struct sockaddr *) &saddr, sizeof(saddr) );
	if( result == -1 ){
		printf("Error in establishing a connection to %s on port %s.\n", argv[1], argv[2]);

		if( close(s) != 0 ){
			printf("Error in closing socket.\n");
			return -3;
		}
		return -4;
	}
	
	for (k=3; k<argc; k++){
		
		// alloco dinamicamente un buffer in cui faccio la richiesta 'GET filename\r\n'
		
		bufferToSend = malloc(4 + strlen(argv[k])*sizeof(char) + 2);
		strcpy(bufferToSend, "GET ");
		strcat(bufferToSend, argv[k]);
		bufferToSend += 4 + strlen(argv[k]);
		*(bufferToSend)='\r';
		*(bufferToSend + 1) = '\n';
		bufferToSend -= strlen(argv[k]) + 4;
		
		// 1 - invio il buffer al server in cui chiedo il file
		if( ( send( s, bufferToSend, (size_t)(4 + strlen(argv[k])*sizeof(char)+2 ), 0 )) != (size_t)(4 + strlen(argv[k])*sizeof(char) +2) ){
			printf("Error in sending parameters to server.\n");
			free(bufferToSend);
			return -5;
		}
		memset(answer, 0, BUFF_DIM);
		
		
		fileDimension=0;
		fileLastModified=0;
		end=0;
		bytes_rec=0;
		tot=0;

		// timer 
		FD_ZERO(&cset);
		tval.tv_sec= t;
		tval.tv_usec= 0;
		FD_SET(s, &cset);

			if ( (n= select(FD_SETSIZE, &cset, NULL, NULL, &tval) )== -1){
				printf("Errore in select()\n");
				close(s);
				free(bufferToSend);
				return -6;

			} else if (n==0){
				printf ("Timeout %d secs expired. Try again. \n", t);
				close(s);
				free(bufferToSend);				
				return -7;
			} else {

				 // 1 -  +OK\r\nBYTES
				int val = recv(s, answer, 5*sizeof(char) + sizeof(uint32_t), 0);
				if (val==0){
					printf("Connection closed.\n");
					close(s);
					free(bufferToSend);
					return -8;

				} else if (val<0){
					printf ("Error in receiving bytes from server.\n");
					close(s);
					free(bufferToSend);
					return -9;
				} else if (strcmp((const char*)answer, error)==0){
					// errore da parte del server
						printf("%s",error);
						close(s);
						free(bufferToSend);
						return -4;
				}

				 // 2 - +OK\r\n.B1.B2.B3.B4
				for (i=0, j=5; i<4; i++, j++){  
					fileDimension += answer[j];
					if (i<3){
						fileDimension <<=8;
					}
				}

				//printf("Dimensione file '%s' : %d  bytes.\n", argv[k], fileDimension);
				memset(answer, 0, BUFF_DIM);

				// 3 - New file or open the existing one to update it
				if ( (file_toWrite = fopen(argv[k], "wb"))==NULL){ 
					printf("Error in opening file.\n");
					free(bufferToSend);
					close(s);
					return -11;
				}

				while(end==0){

					if (tot+BUFF_DIM>= fileDimension){
							toRead = fileDimension-tot;
							end=1;
					} else {
							toRead = BUFF_DIM;
					}

					FD_ZERO(&cset);
					tval.tv_sec= t;
					tval.tv_usec= 0;
					FD_SET(s, &cset);

					if ( (n= select(FD_SETSIZE, &cset, NULL, NULL, &tval) )== -1){
						printf("Errore in select()\n");
						close(s);
						free(bufferToSend);
						return -6;

					} else if (n==0){
						printf ("Timeout %d secs expired. Try again. \n ", t);
						close(s);
						free(bufferToSend);
						return -7;
					}

					// 4 - Receive the bytes of the file
					val = recv(s, answer, toRead, 0);
					if (val==0){
						printf("La connessione e' stata chiusa.\n");
						close(s);
						free(bufferToSend);						
						return -8;
					} else if (val<0){
						printf ("Error in receiving bytes from server.\n");
						close(s);
						free(bufferToSend);						
						return -9;
					} else if (strcmp((const char*)answer, error)==0){
							printf("%s", answer);
							close(s);
							free(bufferToSend);
							return -4;
					}
					
					// 5 - Writes the bytes received in the file 
					if ( (written= fwrite(answer, sizeof(char), val, file_toWrite)) != val ){
						printf("Error in writing on file \n");
						free(bufferToSend);					
						return -11;
					}
					tot +=val;
					memset(answer, 0, BUFF_DIM);
			
				}
				fclose(file_toWrite);
				
				// 6 - Timestamp
				FD_ZERO(&cset);
				tval.tv_sec= t;
				tval.tv_usec= 0;
				FD_SET(s, &cset);

				if ( (n= select(FD_SETSIZE, &cset, NULL, NULL, &tval) )== -1){
					printf("Errore in select()\n");
					close(s);					
					free(bufferToSend);
					return -6;

				} else if (n==0){
					printf ("Timeout %d secs expired. Try again. \n ", t);
					close(s);
					free(bufferToSend);
					return -7;
				}
				val = recv(s, answer, sizeof(uint32_t), 0);
				if (val==0){
					printf("La connessione e' stata chiusa.\n");
					close(s);
					free(bufferToSend);					
					return -8;
				} else if (val<0){
					printf ("Error in receiving bytes from server.\n");
					close(s);
					free(bufferToSend);					
					return -9;
				} else if (strcmp((const char*)answer, error)==0){
						printf("%s", answer);
						close(s);
						free(bufferToSend);
						return -4;
				}
				
				for(j=0; j<4; j++){
					fileLastModified += answer[j];
					if (j<3){
						fileLastModified <<= 8;
					}
				}
				
				free(bufferToSend);
				memset(answer, 0, BUFF_DIM);
				
				printf("\nFile %s downloaded: %d bytes, %d timestamp.\n", argv[k], fileDimension, fileLastModified);
		}
	}
	close(s);
	return 0;
}

