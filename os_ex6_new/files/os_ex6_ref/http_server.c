#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <dirent.h>
#include <fcntl.h>


/******************************* globals & definitions *************************/
pthread_t thread_id[1000];							// saves thread_is
int servSd, numOfThreads, maxRequests;						// server's socket, number of threads, max requests
pthread_mutex_t mutex;								// mutex for queue actions
pthread_cond_t cond;								// cond for mutual queue acess. 
										// will get a signal when deque is avaiable
	
struct queue_elem{								// queue defenition 

	int sd;									//socket descriptor
	struct queue_elem* next;						//pointer to next element
};

typedef struct queue_elem ELEMENT;
typedef ELEMENT* LINK;

LINK head = NULL, tail = NULL;							// head, tail, and size for queue
int queueSize = 0;

int done =0; //to signal ctrl+c	



/******************************* queueUtils ***********************************/
/*	------		------		------
	|tail|----->	|    |	----->	|head|
	------		------		------

	enquing before the tail(end of the queue)
	dequing head
*/


/* priniting queue from end to start. for self use */
void printQueue(){

	LINK tmp = tail;
	printf("Printing queue(sd's) from tail to head:\n");

	while(tmp!=head){
		printf("...%d...\n", tmp->sd);
		tmp=tmp->next;
	}
	if(head!=NULL)
		printf("...%d...\n", tmp->sd);
}



/* adds socket to end of queue
   return -1 if queue is full, 0 on success */
int enqueue(int cliSd){
	
	LINK tmp = (LINK)malloc(sizeof(ELEMENT));				// crating element to add to queue

	if(tmp == NULL){
		printf("Error : Stanadrt function malloc has failed\n");
		exit(0);
	}

	tmp->sd = cliSd;
	int rc, ret = 0;							// ret is return value. will remain 0 on success
	rc = pthread_mutex_lock(&mutex);
	if(rc){
		printf("Error : lock() failed\n");
		exit(0);
	}
	
	if(queueSize >= maxRequests){						// rejecting a client since queue is full
		free(tmp);
		ret = -1;
	}
	else{									// adding client to queue
		tmp->next = tail;
		*(&tail) = tmp;

		if(queueSize == 0)						// if only 1 element : head = tail
			*(&head) = tmp;
		
		queueSize++;
		rc = pthread_cond_signal(&cond);				// signal dequeue is available
		if(rc){
			printf("Error : cond_signal() failed\n");
			rc = pthread_mutex_unlock(&mutex);
			if(rc)
				printf("Error : unlock() failed\n");
			exit(0);
		}
	}
	rc = pthread_mutex_unlock(&mutex);
	if(rc){
		printf("Error : unlock() failed\n");
		exit(0);
	}
	return ret;
}


/* dequeue from queue.
	returns next socket on success, -1 on end (signals for thread to exit) */
int deque(){

	int ret;	
						// return value : the next socket
	int rc = pthread_mutex_lock(&mutex);
	if(rc){
		printf("Error : lock() failed\n");
		exit(0);
	}

	if(queueSize == 0 && !done){							//if we have what to consume or that we are done - we wont wait!
		rc = pthread_cond_wait(&cond,&mutex);					// waiting for signal that dequeue is available
		if(rc){
			printf("Error : cond_wait() failed\n");
			rc = pthread_mutex_unlock(&mutex);
			if(rc)
				printf("Error : unlock() failed\n");
			exit(0);
		}
	}
	if(queueSize>0){
		LINK tmp = tail;

		if(queueSize == 1){							// now queue is empty
			ret = tmp->sd;
			*(&head) = NULL;
			*(&tail) = NULL;
			free(tmp);
		}
		else{									// if queue not of size 1 - removing head
			while(tmp->next != head)
				tmp = tmp->next;
			ret = head->sd;
			free(head);
			*(&head) = tmp;
		}
		queueSize--;
	}
	else										// done! gracefully shoud exit thread
		ret = -1;

	rc = pthread_mutex_unlock(&mutex);
	if(rc){
		printf("Error : unlock() failed\n");
		exit(0);
	}	
	return ret;
}



/* main function for each working thread :
	deques and handles request */
void* workerThread(void* args){
	
	while(1){
		int cliSd = deque();

		if(cliSd == -1)				// we got signaled to gracefully exit
			return NULL;
				
		char buf[1044]={0};			// 1024 of path + other parameters of first line in request
		int n;

		n = read(cliSd, buf, sizeof(buf)-1);
		if(n==0){				// case eof (not http get or post)
			if(close(cliSd)<0)
				printf("Error : close() has failed\n");
			continue;
		}
		if(n < 0){
			printf("Error : failed to recv a message\n");
			exit(0);
		}
		buf[n]=0;

		char method[10], path[1024], http[10];

		if( sscanf(buf, "%s %s %s", method, path, http) != 3 ){
			printf("Error : Standart function sscanf has failed\n");
			exit(0);	
		}

		if(strcasecmp(method, "GET")!=0 && strcasecmp(method, "POST")!=0){				
			char* cliBuf = "HTTP/1.0 501 Not Implemented\r\n\r\nNot Imeplemented(501)";
			if ( sendAll(cliSd, cliBuf, strlen(cliBuf)) == -1 ){
				printf("Error : failed to send a message\n");
				exit(0);
			}
		}
		else{					//get or post

			struct stat info;

			int stat_val = stat(path, &info);
			if(stat_val == -1 && errno != ENOENT){
				printf("Error : stat() failed\n");
				exit(0);
			}
			else if(stat_val == -1 && errno == ENOENT){			// File not found
				char* cliBuf = "HTTP/1.0 404 Not Found\r\n\r\nNot Found(404)";
				if ( sendAll(cliSd, cliBuf, strlen(cliBuf)) == -1 ){
					printf("Error : failed to send a message\n");
					exit(0);
				}
			}
			else if(S_ISDIR(info.st_mode)){					//Directory
				char* cliBuf = "HTTP/1.0 200 OK\r\n\r\n";
				
				if ( sendAll(cliSd, cliBuf, strlen(cliBuf)) == -1 ){
					printf("Error : failed to send a message\n");
					exit(0);
				}
				cliBuf = "\n";

				DIR *dfd;
				struct dirent *dp;

				dfd = opendir(path);
				if(dfd == NULL){
					printf("Error : opendir() failed\n");
					exit(0);
				}
				
				while( (dp = readdir(dfd)) != NULL){
					if ( sendAll(cliSd, dp->d_name, strlen(dp->d_name)) == -1 ){
						printf("Error : failed to send a message\n");
						exit(0);
					}	
					if ( sendAll(cliSd, cliBuf, strlen(cliBuf)) == -1 ){
						printf("Error : failed to send a message\n");
						exit(0);
					}
				}
				closedir(dfd);					

			}
			else{								//File
				char* cliBuf = "HTTP/1.0 200 OK\r\n\r\n";

				if ( sendAll(cliSd, cliBuf, strlen(cliBuf)) == -1 ){
					printf("Error : failed to send a message\n");
					exit(0);
				}

				int fd = open(path, O_RDONLY);
				if (fd == -1){
					printf("Error : failed to open file\n");
					exit(0);
				}

				char buf[1024];
				int seek = 0, count;
				while ( (count  = read(fd, buf, 1024)) != 0 ){
					if( count == -1){
      						printf( "Error reading file: %s\n", strerror( errno ) );
        					exit(0);
   					}

					seek += count;
					if ( lseek(fd, seek, SEEK_SET) < 0 ){
      						printf( "Error seeking file: %s\n", strerror( errno ) );
        					exit(0);
   					} 
					
					if ( sendAll(cliSd, buf, count) == -1 ){
						printf("Error : failed to send a message\n");
						exit(0);
					}
				}
				if(close(fd)<0)
					printf("Error : failed to close file\n");
				

			}
			
		}
	
		if(close(cliSd)<0)
			printf("Error : Failed to close client's socket\n");	

	}

}


/* function to end program on ctrl+c */

void endProgram(int signum){
	done = 1;			// so others will know to gracefully exit	

	int rc;
	int i;				// signaling all threads that they can consume so that they all wake up
					// because done == 1 they wont sllep again, and will finish when queue is empty
	for(i=0;i<numOfThreads;i++){
		rc = pthread_cond_signal(&cond);
		if(rc){
			printf("Error : pthread_cond_signal() failed\n");
			exit(0);
		}
	}
	for(i=0;i<numOfThreads;i++){	// waiting for all threads to return
		rc = pthread_join(thread_id[i], NULL);
		if(rc){
			printf("Error : pthread_join() failed\n");
			exit(0);
		}
	}
		
	if(close(servSd)< 0)
		printf("Error : Failed to close Server's socket\n");

	// destroying mutex and condition
   	if(pthread_mutex_destroy(&mutex))
		printf("Error: pthread_mutex_destroy() has failed\n");
   	if(pthread_cond_destroy(&cond))
		printf("Error: pthread_cond_destroy() has failed\n");

	exit(0);
}






/* Functions to send a specific message.
   Recieves a sockes descreptor , buffer to send, and length of message
   Returns : 0 on success , -1 on failure */

int sendAll (int sock, char* buf, int len){

	int totalSent = 0, bytesLeft = len, n;
	
	// sending while we havent sent all bytes 
	while(totalSent <len){
		if (( n = send(sock, buf + totalSent, bytesLeft, 0) ) == -1)
			return -1;		//failure
		totalSent += n;
		bytesLeft -= n;
	}
	
	return 0;				//sucess
}





int main (int argc, char** argv){

	int cliSd, i=0;
	struct sockaddr_in servAddr, cliAddr;
	socklen_t sin_size = sizeof(struct sockaddr_in);
	uint16_t port;

	char* servBuf, *cliBuf;
	
	if (argc !=3 && argc != 4){
		printf("Usage : http_server numOfThreads maxRequest [port]\n");
		return -1;
	}
	
	if (argc == 4)					// setting port 
		port = atoi(argv[3]);
	else 
		port = 80;

	char* ptr;
	numOfThreads = strtol(argv[1], &ptr, 10);	// getting args of input
	maxRequests = strtol(argv[2], &ptr, 10);


	struct sigaction newAction;			// redirecting ctrl+c to function endProgram
	newAction.sa_handler = endProgram;	
	if(sigaction(SIGINT, &newAction, NULL) < 0 ){
		perror("Error : sigaction() failed\n");
		return -1;
	}


	servSd = socket(AF_INET, SOCK_STREAM, 0);	// handling connection to client
	if (servSd == -1){
		printf("Error : Failed to open socket\n");
		return -1;
	}

	servAddr.sin_family  = AF_INET;
	servAddr.sin_port = htons(port);
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(servSd, (struct sockaddr*)&servAddr, sizeof(servAddr))){
		printf("Error : Failed to bind socket\n");
		if(close(servSd)<0)
			printf("Error : close() has failed\n");
		return -1;
	}
	
	
	if (listen(servSd, 10)){
		printf("Error : Function listen() has failed\n");
		if(close(servSd)<0)
			printf("Error : close() has failed\n");
		return -1;
	}

	int rc;						// initializing mutex & cond
   	rc = pthread_mutex_init(&mutex, NULL); 
	if(rc!=0){
		printf("Error : mutex_init() has failed\n");
		if(close(servSd)<0)
			printf("Error : close() has failed\n");
		return -1;
	}

   	rc = pthread_cond_init(&cond, NULL);
	if(rc!=0){
		printf("Error : cond_init() has failed\n");
		if(close(servSd)<0)
			printf("Error : close() has failed\n");
		return -1;
	}

	for(i=0;i<numOfThreads;i++){			// creating working threads
		
		if(pthread_create( &thread_id[i] , NULL ,  workerThread , NULL ) < 0){
       				printf("Error : could not create thread\n");
				if(close(servSd)<0)
					printf("Error : close() has failed\n");
           			exit(0);
       		}
	}

	while(1){					// getting connections and enqueing
		cliSd = accept(servSd, (struct sockaddr*)&cliAddr, &sin_size);
		if(cliSd < 0){
			printf("Error : Failed to accept a connection\n");
			if(close(servSd)<0)
				printf("Error : close() has failed\n");
			exit(0);
		}

		if(enqueue(cliSd) == -1 ){
			cliBuf = "HTTP/1.0 503 Service Unavailable\r\n\r\nService Unavailable(503)";
			if ( sendAll(cliSd, cliBuf, strlen(cliBuf)) == -1 ){
				printf("Error : failed to send a message\n");
				if(close(servSd)<0)
					printf("Error : close() has failed\n");
				exit(0);
			}
			if(close(cliSd)<0)
				printf("Error : close() has failed\n");
		}

	}

}