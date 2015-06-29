#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 
#include <errno.h>
#include <assert.h>

#define DEBUG 0

//just print. not interesting
void print_sock_details(int sockfd){
	/* print socket details again */
	if (DEBUG){
		struct sockaddr_in my_addr, peer_addr;  
		socklen_t addrsize = sizeof(struct sockaddr_in );

		getsockname(sockfd, (struct sockaddr*)&my_addr, &addrsize);
		getpeername(sockfd, (struct sockaddr*)&peer_addr, &addrsize);
		printf("connected! in my socket i am %s:%d (peer is %s:%d)\n", 
		inet_ntoa((my_addr.sin_addr)),  ntohs(my_addr.sin_port),
		inet_ntoa((peer_addr.sin_addr)),  ntohs(peer_addr.sin_port));
	}
}


//init the sockaddr_in var -  TODO : dns don't work (should?)
void init_sockaddr(struct sockaddr_in *name,
					const char *hostname, unsigned short int port){
	struct hostent *hostinfo;

	name->sin_family = AF_INET;
	name->sin_port = htons (port);
	hostinfo = gethostbyname (hostname);
	if (hostinfo == NULL) 
	{
		fprintf (stderr, "Unknown host %s.\n", hostname);
		exit (EXIT_FAILURE);
	}
	name->sin_addr = *(struct in_addr *) hostinfo->h_addr;
}
