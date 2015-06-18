
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

int main(int argc, char *argv[])
{
    int sockfd = 0, n = 0;
    char recvBuff[1024];
    struct sockaddr_in serv_addr; 
    struct sockaddr_in my_addr, peer_addr;  
    socklen_t addrsize = sizeof(struct sockaddr_in );

    memset(recvBuff, '0',sizeof(recvBuff));
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        return 1;
    } 

    /* print socket details */
    getsockname(sockfd, (struct sockaddr*)&my_addr, &addrsize);
    printf("initially in my socket i am %s:%d)\n", 
            inet_ntoa((my_addr.sin_addr)),  ntohs(my_addr.sin_port));        

    memset(&serv_addr, '0', sizeof(serv_addr)); 

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[1])); // Note: htons for endiannes
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // hardcoded...

    printf("connecting...\n");
    /* Note: what about the client port number? */ 
    /* connect socket to the above address */   
    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
       printf("\n Error : Connect Failed. %s \n", strerror(errno));
       return 1;
    } 

    /* print socket details again */
    getsockname(sockfd, (struct sockaddr*)&my_addr, &addrsize);
    getpeername(sockfd, (struct sockaddr*)&peer_addr, &addrsize);
    printf("connected! in my socket i am %s:%d (peer is %s:%d)\n", 
            inet_ntoa((my_addr.sin_addr)),  ntohs(my_addr.sin_port),
            inet_ntoa((peer_addr.sin_addr)),  ntohs(peer_addr.sin_port));

    /* read data from server into recvBufff
       block until there's something to read
	   print data to screen every time*/   
    while ( (n = read(sockfd, recvBuff, sizeof(recvBuff)-1)) > 0)
    {
        recvBuff[n] = 0;
        if(fputs(recvBuff, stdout) == EOF)
        {
            printf("\n Error : Fputs error\n");
        }
    } 

    if(n < 0)
    {
        perror("\n Read error \n");
    } 

    close(sockfd); // is socket really done here?
    //printf("Write after close returns %d\n", write(sockfd, recvBuff, 1));
    return 0;
}
