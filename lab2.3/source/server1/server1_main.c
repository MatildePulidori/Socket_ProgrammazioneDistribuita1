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

#define LISTEN_BACKLOG 15
#define MAX_LENGHT_FILENAME 255
#define MAX_INPUT 6

char *prog_name;

int main (int argc, char *argv[])
{
	int s;
	int sock;
	socklen_t serverAddrLength;
	struct sockaddr_in serverAddr;

	char buffer[MAX_INPUT + MAX_LENGHT_FILENAME];
	int sizeMsgReceived;
	char* ptr;
	char fname[MAX_LENGHT_FILENAME+1];
	int count = 0;

	if ( (s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) <0){
		printf("Error in creating socket \n");
		return -1;
	}
	printf("CIAO\n");
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(atoi(argv[1])); //host to network short per la porta
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); //host to network long per l'indirizzo
	serverAddrLength = sizeof(serverAddr);

	if ( bind (s, (const struct sockaddr * ) &serverAddr, serverAddrLength) <0){
		printf("Error in binding and port. \n");
		return -2;
	}

	if ( listen(s, LISTEN_BACKLOG) < 0 ){
		printf("Error in listening. \n");
		return -3;
	}

	while (1){
		if ( (sock = accept(s, (struct sockaddr *) &serverAddr, &serverAddrLength)) <0){
			printf("Connection not accepted already. \n");
			continue;
		}

		sizeMsgReceived = recv(sock, buffer, (size_t)MAX_INPUT+MAX_LENGHT_FILENAME, 0);
		printf("%d\n", sizeMsgReceived );
		if ( sizeMsgReceived <0){  //Errore nella ricezione
			printf("Error in receiving data.\n");
			continue;
		}
		if (sizeMsgReceived == 0){ //Non c'è più nulla da leggere
			printf("No more data to read and client closed the connection of this socket. \n");
			continue;
		}


		ptr = strstr(buffer, "GET");

		if (ptr == NULL){
			printf("Missing GET command. \n");
			// TO DO -> -ERRCRLF
			continue;

		} else if (ptr != buffer){
			printf("GET command is not in the start position. \n");
			//TO DO --> -ERRCRLF
			continue;
		}
		ptr = ptr +3;

		// ptr adesso dovrebbe puntare allo spazio
		if (*ptr != ' '){
			printf("Missing space between GET and filename. \n ");
			continue;
		}

		for(; *ptr!='\r' && *(ptr)!='\n' && count<MAX_LENGHT_FILENAME;  ptr++, count++);
		if(count>=MAX_LENGHT_FILENAME){
			printf("Unable to find CR-LF \n");
			// TO DO --> -ERRCRLF
			continue;
		}

		*(ptr+2)='\0';
		ptr-=count;

		if ( sscanf(ptr, "%s ", fname) != 1){
			printf ("Error in filename. \n");
			continue;
		}

		// In fname there is the name of the file the server have to send to the client
		printf("Il file si chiama %s \n", fname);


	}


	return 0;
}
