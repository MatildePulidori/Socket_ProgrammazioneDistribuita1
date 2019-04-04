/*
 *  warning: this is just a sample server to permit testing the clients; it is not as optimized or robust as it should be
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

//#define STATE_OK 0x00
//#define STATE_V  0x01

#ifdef TRACE
#define trace(x) x
#else
#define trace(x)
#endif

int main (int argc, char *argv[]) {

	int s;
	struct sockaddr_in echoServAddr;
	struct sockaddr_in echoClientAddr;
	unsigned int clientAddrLen;
	uint32_t tempoClient;
	uint32_t tempoServer;
	uint64_t toSend;

	int receiveMsgSize;

	s = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if( s < 0 )
	{
		printf("Error in creating socket.\n");
		return -1;
	}

	echoServAddr.sin_family = AF_INET;
	echoServAddr.sin_port = htons( atoi(argv[1]) );
	echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if( bind( s, (const struct sockaddr *) &echoServAddr, sizeof(echoServAddr) ) < 0 )
	{
		printf("Error in bind().\n");
		return -1;
	}

	while(1)
	{
		printf("Waiting for packets...\n");
		clientAddrLen = sizeof(echoClientAddr);

		receiveMsgSize = recvfrom( s, &tempoClient, sizeof(tempoClient), 0, (struct sockaddr *) &echoClientAddr, &clientAddrLen );
		if(receiveMsgSize!= sizeof(tempoClient)){
				printf("Error: expected 4 bytes, but received %d bytes \n", receiveMsgSize);
				continue;
		}
		//tempoClient = ntohl((uint32_t)tempoClient);
		printf("Received time of the client:  %u \n", ntohl((uint32_t)tempoClient));
		sleep(4);
		tempoServer = htonl((uint32_t)time(NULL));

		printf("Time of the server to send:  %u \n", ntohl((uint32_t)tempoServer));


		toSend = tempoClient;
		toSend = toSend << 32;
		toSend = toSend + tempoServer;


		if( sendto( s, &toSend , sizeof(toSend), 0, (struct sockaddr *) &echoClientAddr, sizeof(echoClientAddr) ) != sizeof(toSend) )
			printf("Error in sending response %llu.\n", toSend);
	}

}
