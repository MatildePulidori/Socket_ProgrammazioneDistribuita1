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

#include <string.h>
#include <time.h>
#include <unistd.h>

#define BUFF_DIM 4096

char *prog_name;

int main (int argc, char *argv[])
{
	struct sockaddr_in saddr;
	int s;
	int result;
	char *bufferToSend;
	uint8_t answer[BUFF_DIM];

	FILE *file_toWrite;
	int bytes_rec = 0, tot=0, end=0, toRead=0;
	uint32_t fileDimension=0, fileLastModified=0;


	if (argc != 4){
		printf("Error in parameters.\n");
		return -1;
	}

	s = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
	if( s < 0 )
	{
		printf("Error in creating socket.\n");
		return -2;
	}

	// Socke info
	saddr.sin_family = AF_INET;
	if( inet_aton(argv[1],&(saddr.sin_addr)) == 0 ){
		printf("Error in inet_aton() for address %s.\n", argv[1]);
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

	bufferToSend = malloc(4 + strlen(argv[3])*sizeof(char) + 2);
	strcpy(bufferToSend, "GET ");
	strcat(bufferToSend, argv[3]);
	bufferToSend += 4 + strlen(argv[3]);
	*(bufferToSend)='\r';
	*(bufferToSend + 1) = '\n';
	bufferToSend -= strlen(argv[3]) + 4;

	if( ( send( s, bufferToSend, (size_t)(strlen(bufferToSend)), 0 )) != (size_t) strlen(bufferToSend) ){
		printf("Error in sending parameters to server.\n");
		return -1;
	}


	int val = recv(s, answer, 5*sizeof(char) + sizeof(uint32_t), 0); // 1- Receive +OK\r\nBYTES
	if (val==0){
		printf("La connessione e' stata chiusa.\n");
		return -4;

	} if (val<0){
		printf ("Error in receiving bytes from server.\n");
		return -5;
	}
	bytes_rec += val;

	for (int i=0, j=5; i<4; i++, j++){  // Mem the file dimension in a variable
		fileDimension += answer[j];
		if (i<3){
			fileDimension <<=8;
		}
	}
	printf("File %d  bytes \n", fileDimension);
	memset(answer, 0, BUFF_DIM);

	if ( (file_toWrite = fopen(argv[3], "wb"))==NULL){ // 2 - Open a new file to write (or update if it exists already)
		printf("Error in opening file.\n");
		free(bufferToSend);
		return -6;
	}
	while(end==0){
		if (tot+BUFF_DIM>= fileDimension){
				toRead = fileDimension-tot;
				end=1;
		} else {
			toRead = BUFF_DIM;
		}
		val = recv(s, answer, toRead, 0);
		if (val==0){
			printf("La connessione e' stata chiusa.\n");
			return -4;

		} if (val<0){
			printf ("Error in receiving bytes from server.\n");
			return -5;
		}

		if ( 	fwrite(answer, sizeof(char), val, file_toWrite) != val ){
			printf("Error in writing on file \n");
		}
		tot +=val;
		bytes_rec += val;
		memset(answer, 0, BUFF_DIM);

	}

	free(bufferToSend);
	fclose(file_toWrite);
	close(s);

return 0;
}
