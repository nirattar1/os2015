/**
* File:			setos_client.c
* Author:		Moshe Sulamy
* Description:	Source file of sample client for setos_server
*				for OS course 15b at Tel-Aviv University
**/

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

#define NUM_NUMS	10
#define MAX_NUM		1000000

int main(int argc, char *argv[])
{
    int sockfd, i, rc;
    char buff[5];
    struct sockaddr_in serv_addr; 
    socklen_t addrsize = sizeof(struct sockaddr_in );
	time_t t;
	srand((unsigned) time(&t));

    //memset(buff, '0',sizeof(recvBuff));
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("error! socket() failed: %s\n", strerror(errno));
        return 1;
    } 

    memset(&serv_addr, '0', sizeof(serv_addr)); 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));		// Note: htons for endiannes
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);

    /* connect socket to the above address */   
	printf("connecting...\n");
    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
       printf("error! connect() failed: %s\n", strerror(errno));
       return 1;
    }
    printf("connected!\n");

	for (i = 0; i < NUM_NUMS; ++i)
	{
		char op = rand() % 3;
		int num = abs(rand() % MAX_NUM);
		printf("op: %d, num: %d\n", op, num);

		rc = write(sockfd, &op, 1);
		if (rc != 1)
		{
			printf("error! write() failed: %s\n", strerror(errno));
			break;
		}
		
		uint32_t net_num = htonl(num);
		int nsent = 0;
		while (nsent < 4)
		{
			rc = write(sockfd, &net_num + nsent, 4 - nsent);
			if (rc <= 0)
			{
				printf("error! write() failed: %s\n", strerror(errno));
				break;
			}
			
			nsent += rc;
		}
		
		if (rc <= 0)
			break;
	}

	printf("closing socket\n");
    close(sockfd);
    return 0;
}
