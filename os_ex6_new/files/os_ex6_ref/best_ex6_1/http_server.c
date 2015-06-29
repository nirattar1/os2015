#include <pthread.h>

#include "my_lib.h"


//put inside path the resource requested by the client connected to connfd
void read_request(int connfd, char* path, char* method){
	char version[20];
	char recvBuff[1025];
	memset(recvBuff, 0, sizeof(recvBuff)); 
	int n = read(connfd, recvBuff, sizeof(recvBuff)-1);
	if(n < 0)
	{
		perror("\n Read error \n");
	} 
	printf("%s\n",recvBuff);
	assert(sscanf(recvBuff, "%s /%s %s\r\n", method, path,version) == 3);
}


void send_to_client(int connfd, char* line){
	int nsent, totalsent;
	int notwritten = strlen(line);
	
	/* keep looping until nothing left to write*/
	totalsent = 0;
	while (notwritten > 0){
		/* notwritten = how much we have left to write
			totalsent  = how much we've written so far
			nsent = how much we've written in last write() call */
		nsent = write(connfd, line + totalsent, notwritten);
		assert(nsent>=0); // check if error occured (client closed connection?)
		
		totalsent  += nsent;
		notwritten -= nsent;
	}
}



//read file using fd and write to soccet connfd
void send_content(int fd, int connfd){
	char line[1024];
	memset(line, 0, sizeof(line));

	send_to_client(connfd, "HTTP/1.0 200 OK\r\n\r\n");
	
	int return_value;
	while ((return_value=read(fd, line, 1024)) > 0) {
		send_to_client(connfd, line);
		memset(line, 0, sizeof(line));
	}
	if (return_value < 0){
		perror("Error reading local resource");
		send_to_client(connfd, "An error occurred while trying to read the resource. See server logs\n"); 
		return;
	}
	
}


void *thread(void* i){
	/* read request from client */	
	int connfd = *((int*) i);
	int fd;
	
	char method[20];
	char path[1000];
	read_request(connfd,path,method);	
	if( strcmp("GET",method) && strcmp("POST",method)){
		send_to_client(connfd,"HTTP/1.0 501 Not Implemented\r\n\r\n");
		send_to_client(connfd,"<div>This method is not implemented</div>\n");
		close(connfd);
		return;
	}
	/* open resource */
	if ((fd = open(path,O_RDONLY))==-1){
		perror("Can't open requested file"); 
		send_to_client(connfd,"HTTP/1.0 404 Not Found\r\n\r\n");
		send_to_client(connfd,"<div>File not Found</div>\n");
		close(connfd);
		return;
	}
	//TODO optional try to read from fd before sending 200 OK / or make sure it is a file and not a directory
	
	/* send content to client */
	send_content(fd, connfd);
	
	/* close resources */
	close(connfd);
	close(fd);
}




//main
int main(int argc, char *argv[])
{
	//handle arguments
	int port = (argc > 1 ? atoi(argv[1]) : 80);
	
	//declare vars
	int listenfd = 0, connfd = 0;
	struct thread_args *args;

	
	//server details
	struct sockaddr_in serv_addr; 
	memset(&serv_addr, '0', sizeof(serv_addr));

	/* create listen socket */
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	init_sockaddr(&serv_addr,"127.0.0.1",port);
	
	/* bind */
	if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))){
		perror ("Bind (server)");
		exit (EXIT_FAILURE);
	}

	/* listen */
	if(listen(listenfd, 10)){
		perror ("Listen (server)");
		exit (EXIT_FAILURE);	
	}

	while(1)
	{
		/* accpeting connection */
		connfd = accept(listenfd, NULL, NULL);
		if(connfd<0){
			perror ("Accept (server)");
			exit (EXIT_FAILURE);	
		}
		
		print_sock_details(listenfd);
		print_sock_details(connfd);
	
		pthread_t *threads=(pthread_t*) malloc(sizeof(pthread_t));
		if(threads==NULL){
			perror("malloc pthread_t* (server)"); 
			send_to_client(connfd,"HTTP/1.0 503 Service Unavailable\r\n\r\n");
			send_to_client(connfd,"<html><body><div>Service Unavailable</div></body></html>\n");
			close(connfd);
			continue;
		}
		
		
		if(pthread_create(threads, NULL, thread, &connfd)){
			perror("pthread_create (server)"); 
			send_to_client(connfd,"HTTP/1.0 503 Service Unavailable\r\n\r\n");
			send_to_client(connfd,"<html><body><div>Service Unavailable</div></body></html>\n");
			close(connfd);
		}
	}
}

