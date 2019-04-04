#include <stdlib.h> // getenv()
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h> // timeval
#include <sys/select.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h> // inet_aton()
#include <sys/un.h> // unix sockets
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h> // SCNu16
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>


void itoa(int n, char *s);
void reverse (char *s);

int main(int argc, char ** argv)
{
	struct sockaddr_in saddr;
	struct sockaddr_in fromaddr;
	int s;
	//char buf[100];
	unsigned int fromlen;
	fd_set cset;
	struct timeval tval;
	int n_times = 5;
	int correct = 0;

	uint32_t tempo;
	uint64_t resp;



	s = socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP);

	if( s < 0 )
	{
		printf("Error in creating socket.\n");
		return -1;
	}
	saddr.sin_family = AF_INET;

	if( inet_aton(argv[1],&(saddr.sin_addr)) == 0 )
		printf("Error in inet_aton() for address %s.\n", argv[1]);

	/*
	if( inet_aton(argv[2],&(saddr.sin_port)) == 0 )
		printf("Error in inet_aton() for port %s.\n", argv[2]);
	*/

	saddr.sin_port = htons( atoi(argv[2]) );

	// scrivere tempo
	numerotempo  = htonl((unsigned int)time(NULL));


	// invio datagramma
	if( sendto( s, &tempo, (size_t) sizeof(tempo), 0, (const struct sockaddr *)&saddr, sizeof(saddr)) != (size_t) sizeof(tempo) )
	{
		printf("Error in sending parameters to server.\n");
		return -1;
	}

	// stampa di cio che ho inviato
	printf("Inviato tempo : %d \n", (int) time(NULL));


	// controllo di ricevere qualcosa entro un timeout di 3 secondi, per al massimo 4 volte
	FD_ZERO(&cset);
	int t = 3;
	int n;
	tval.tv_sec= t;
	tval.tv_usec= 0;

	while( correct == 0 && n_times > 0 ){

			FD_SET(s, &cset);
			if ( (n= select(FD_SETSIZE, &cset, NULL, NULL, &tval) )== -1){
				printf("Errore in select()\n");
				return -5;

			} if (n==0){
				printf ("Timeout %d secs expired. Try again. \n ", t);
				n_times--;
				printf("You can try only %d times. \n", n_times);

			}
			else {

					// ricezione
					fromlen = sizeof(fromaddr);
					if( recvfrom( s, &resp, (size_t) sizeof(resp), 0, (struct sockaddr *)&fromaddr, &fromlen) < 0 )
					{
						printf("Error in receiving data from server.\n");
						return -4;
					}
					// stampa cio che viene ricevuto
					printf("Response %llu\n", resp );
					correct = 1;
				}
}
	if( close(s) != 0 )
	{
		printf("Error in closing socket.\n");
		return -2;
	}

	return 0;
}
