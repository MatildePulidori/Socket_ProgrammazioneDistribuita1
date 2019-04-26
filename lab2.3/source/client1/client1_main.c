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

char *prog_name;

int main (int argc, char *argv[])
{
	struct sockaddr_in saddr;
	int s;
	int result;
	char buf[100];

	s = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
	if( s < 0 )
	{
		printf("Error in creating socket.\n");
		return -1;
	}

	// campi del socket
	saddr.sin_family = AF_INET;
	if( inet_aton(argv[1],&(saddr.sin_addr)) == 0 )
		printf("Error in inet_aton() for address %s.\n", argv[1]);
	saddr.sin_port = htons( atoi(argv[2]) );

	result = connect( s, (struct sockaddr *) &saddr, sizeof(saddr) );
	if( result == -1 ){
		printf("Error in establishing a connection to %s on port %s.\n", argv[1], argv[2]);

		if( close(s) != 0 ){
			printf("Error in closing socket.\n");
			return -2;
		}
		return -3;
	}

	buf[0] = 'G';
	buf[1] = 'E';
	buf[2] = 'T';
	buf[3] = ' ';
	buf[4] = 'a';
	buf[5] = '\r';
	buf[6] = '\n';


	if( send( s, buf, (size_t)(sizeof(buf)), 0 ) != (size_t) sizeof(buf) )
	{
		printf("Error in sending parameters to server.\n");
		return -1;
	}


}
