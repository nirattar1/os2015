#include "my_lib.h"

//send a simple get request for the resource specified by path to this socket.
void request_from_server(int filedes,char* path){
	//int nbytes;
//
//	nbytes = write (filedes, MESSAGE, strlen (MESSAGE) + 1);
//	if (nbytes < 0)
//	{
//		perror ("write");
//		exit (EXIT_FAILURE);
//	}
//	
//		
	char sendBuff[1025];
	memset(sendBuff, 0, sizeof(sendBuff)); 

	snprintf(sendBuff, sizeof(sendBuff), "GET /%s HTTP/1.0\r\n", path);
	
	int nsent,totalsent = 0;
	int notwritten = strlen(sendBuff);

	/* keep looping until nothing left to write*/
	while (notwritten > 0){
		/* notwritten = how much we have left to write
			totalsent  = how much we've written so far
			nsent = how much we've written in last write() call */
		nsent = write(filedes, sendBuff + totalsent, notwritten);
		assert(nsent>=0); // check if error occured (client closed connection?)
		
		totalsent  += nsent;
		notwritten -= nsent;
	}
}


void print_response(int sockfd){
	int n=0;
	char recvBuff[1024];
	memset(recvBuff, '0',sizeof(recvBuff));

		
	/* read data from server into recvBufff
	block until there's something to read
	print data to stdout*/   
	while ( (n = read(sockfd, recvBuff, sizeof(recvBuff)-1)) > 0)
	{
		recvBuff[n] = 0;
		if(fputs(recvBuff, stdout) == EOF)
		{
			perror("\n Error : Fputs error\n");
		}
	}	
	if(n < 0)
	{
		perror("\n Read error \n");
	} 
}



//main
int main(int argc, char *argv[])
{
	//handle arguments
	assert(argc > 2);
	char* hostname = argv[1];
	char* path = argv[2];  
	path++;   //Important: I expect the path to be preceded by a single slash. o.w. comment this line; the problem is that with leading slash the console does not intemperate the tilda sign.
	int port = (argc > 3 ? atoi(argv[3]) : 80);
	

	
	//needed vars
	int sockfd;
	struct sockaddr_in serv_addr; 
	
	//why?
	memset(&serv_addr, '0', sizeof(serv_addr)); 
	
	/* Create the socket.*/
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror ("socket (client)");
		exit (EXIT_FAILURE);
	}

	/* Connect to the server.	 */
	init_sockaddr (&serv_addr, hostname, port);
	if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		perror ("connect (client)");
		exit (EXIT_FAILURE);
	} 
	print_sock_details(sockfd);
	
	/* Send request to the server.	 */
	request_from_server(sockfd,path);
	
	/* Read response */  
	print_response(sockfd);
	close(sockfd); // is socket really done here?
	return 0;
}
