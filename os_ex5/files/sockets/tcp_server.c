#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h> 
#include <assert.h>

int main(int argc, char *argv[])
{
    int totalsent, nsent, len, n = 0, listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr, my_addr, peer_addr;  
    char sendBuff[1025];
    time_t ticks; 

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(sendBuff, '0', sizeof(sendBuff)); 

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY = any local machine address
    serv_addr.sin_port = htons(atoi(argv[1])); 

    if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))){
       printf("\n Error : Bind Failed. %s \n", strerror(errno));
       return 1; 
    }

    if(listen(listenfd, 10)){
       printf("\n Error : Listen Failed. %s \n", strerror(errno));
       return 1; 
    }

    while(1)
    {
        /* new connection */
        socklen_t addrsize = sizeof(struct sockaddr_in );

        /* accpeting connection. can use NULL in 2nd and 3rd arguments
           but we want to print the client socket details*/
        connfd = accept(listenfd, (struct sockaddr*)&peer_addr, &addrsize);

        if(connfd<0){
           printf("\n Error : Accept Failed. %s \n", strerror(errno));
           return 1; 
        }

        getsockname(connfd, (struct sockaddr*)&my_addr, &addrsize);
        getpeername(connfd, (struct sockaddr*)&peer_addr, &addrsize);
        printf("connected to client! peer is %s:%d (i am %s:%d)\n", 
            inet_ntoa((peer_addr.sin_addr)), ntohs(peer_addr.sin_port),
            inet_ntoa((my_addr.sin_addr)),   ntohs(my_addr.sin_port));

        /* write time */
        ticks = time(NULL);
        snprintf(sendBuff, sizeof(sendBuff), "%.24s\r\n", ctime(&ticks));

        totalsent = 0;
        int notwritten = strlen(sendBuff);

        /* keep looping until nothing left to write*/
        while (notwritten > 0){
           /* notwritten = how much we have left to write
              totalsent  = how much we've written so far
              nsent = how much we've written in last write() call */
           nsent = write(connfd, sendBuff + totalsent, notwritten);
           assert(nsent>=0); // check if error occured (client closed connection?)
           printf("wrote %d bytes\n", nsent);

           totalsent  += nsent;
           notwritten -= nsent;
        }

        /* close socket  */
        close(connfd);
     }
}
