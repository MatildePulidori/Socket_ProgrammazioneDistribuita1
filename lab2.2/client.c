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

void itoa(int n, char *s);
void reverse (char *s);

int main(int argc, char ** argv)
{	
	struct sockaddr_in saddr;
	struct sockaddr_in fromaddr;
	int s;
	char buf[100];
	unsigned int fromlen;

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
	
	// scrivere nome nel buf
	strcpy(buf , argv[3]);	
	
	// invio datagramma
	if( sendto( s, buf, (size_t) strlen(buf), 0, (const struct sockaddr *)&saddr, sizeof(saddr)) != (size_t) strlen(buf) )
	{
		printf("Error in sending parameters to server.\n");
		return -1;
	}

	// ricezione?
	fromlen = sizeof(fromaddr);
	if( recvfrom( s, buf, (size_t) 100, 0, (struct sockaddr *)&fromaddr, &fromlen) < 0 )
	{
		printf("Error in receiving data from server.\n");
		return -4;
	}
	printf("Response %s\n", buf);

	if( close(s) != 0 )
	{
		printf("Error in closing socket.\n");
		return -2;
	}

	return 0;
}


 
