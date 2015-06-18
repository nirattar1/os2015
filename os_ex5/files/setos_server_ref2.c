#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <arpa/inet.h> 
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include "setos.h"

/************* globals **************/

// mutex and cond variable
pthread_mutex_t sharedMutex; // mutex for access to shared "data"
pthread_cond_t  notEmptyCond; // condition variable to wake up "consumers" of data

// our shared predicates and "data"
int						isEmpty = 1; // used to signal consumers waiting on condition variable
int						isDone = 0;
int						numOfThreads = 0;
int						rc = 0;
struct _linked_list*	lst;

/************************************/


//node list struct
//node for socket queue
typedef struct _list_node{
	int					value;
	struct _list_node*	next;
	struct _list_node*	prev;

}list_node;


//node linked_list
//node for socket queue
typedef struct _linked_list{
	list_node*			head;
	list_node*			tail;
	int					nOfElems;

}linked_list;

int initList(){
	lst				= (linked_list*)malloc(sizeof(linked_list));
	lst->head		= NULL;
	lst->tail		= NULL;
	lst->nOfElems	= 0;

	if (rc) {
		printf("Server: Failed list init, rc=%d.\n", rc);
		return(1);
	}
	return 0;
}

list_node* initNode(int skfd){
	list_node* node		= (list_node*)malloc(sizeof(list_node));
	node->value			= skfd;
	node->next			= NULL;
	node->prev			= NULL;
}

int enqueue(list_node* node){
	rc = pthread_mutex_lock(&sharedMutex);

	if (lst->nOfElems == 0){
		lst->head = node;
		lst->tail = node;
		lst->nOfElems++;
		isEmpty = 0;
	}
	else {
		list_node* prev = lst->tail;
		prev->next = node;
		lst->tail = node;
		node->prev = prev;
		lst->nOfElems++;
	}

	//signal for threads waiting in deque function
	rc = pthread_cond_signal(&notEmptyCond);

	rc = pthread_mutex_unlock(&sharedMutex);
	return 0;
}

list_node* dequeue(){
	list_node* node = NULL;
	rc = pthread_mutex_lock(&sharedMutex);

	while ( isEmpty && !isDone){ 
		//handle SIGINT result while in loop
		rc = pthread_cond_wait(&notEmptyCond, &sharedMutex);

		if (rc) {
			printf("Socket Handler Thread %u: condwait failed, rc=%d\n", (unsigned int)pthread_self(), rc);
			pthread_mutex_unlock(&sharedMutex);
			exit(1);
		}
	}

	if (!isEmpty){ //condition for waking signal after isDone == 1 
		list_node* node = lst->tail;
		lst->tail = node->prev;
		lst->tail->next = NULL;
		lst->nOfElems--;
		isEmpty = lst->nOfElems == 0 ? 1 : 0;
	}

	rc = pthread_mutex_unlock(&sharedMutex);

	return node;

}

void *handleSocket(void *parm){
	list_node* node;
	int decfd;
	char recvBuff[5];
	int notReaden = sizeof(recvBuff);
	int totalRead = 0;
	int nread = 0;
	int isClosed = 0;
  int res;

	while (!isDone || !isEmpty){

		node = dequeue();

		while (!isClosed && node!= NULL){
			decfd = node->value;
			/* keep looping until nothing left to read*/
			while (notReaden > 0){
				/* notReaden = how much we have left to read
				totalread  = how much we've readen so far
				nsread = how much we've reden in last read() call */
				nread = read(decfd, recvBuff + totalRead, notReaden);
				if (nread < 1){ //socket is closed
					close(decfd);
					isClosed = 1;
				}
				totalRead += nread;
				notReaden -= nread;
			}
			if (totalRead == 5){

				if (atoi(recvBuff) == 0)
					res = setos_add(ntohl(*((uint32_t *)(recvBuff + 1))),NULL);
				else if (atoi(recvBuff) == 1)
					res = setos_remove(ntohl(*((uint32_t *)(recvBuff + 1))), NULL);
				else
					res = setos_contains(ntohl(*((uint32_t *)(recvBuff + 1))));

				write(decfd, &res, 1);

				printf("write:Thread %u:	data = %d, op = %d, res =%d\n", (unsigned int)pthread_self(), ntohl(*((uint32_t *)(recvBuff + 1))), atoi(recvBuff), res );
			}
			else{
				printf("Socket Handler Thread %u: read fail,\n", (unsigned int)pthread_self());
			}
		}

		isClosed = 0;
	}
	

	return NULL; // exiting gracefuly
}

static void handler(int signum){
	if (signum == SIGINT)
		isDone = 1;
}

int main(int argc, char **argv){
	int						i; //loop itarating
	int						listenfd = 0;
	int						connfd = 0;
	int						numOfThreads = atoi(argv[1]);
	struct sockaddr_in		serv_addr;
	char					sendBuff = 0;
	pthread_t*				thread;
	struct sigaction sa;

	//signal init
	sa.sa_handler = handler;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);

	//signal handler
	sigaction(SIGINT, &sa, NULL);  //set up isDone to 1
	sigaction(SIGPIPE, &sa, NULL); //is ingnored in function

	//Threads init
	thread = (pthread_t*)malloc(numOfThreads*sizeof(pthread_t));

	/* init mutex and cond variable */
	rc = pthread_mutex_init(&sharedMutex ,NULL); 
	rc = pthread_cond_init(&notEmptyCond, NULL); 

	//Socket List init
	initList();

	//data List Init
	setos_init();

	for (i = 0; i <numOfThreads; i++) {
		rc = pthread_create(&thread[i], NULL, handleSocket, NULL);
	}
	sleep(2); // give time for all handlers to start

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&serv_addr, '0', sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY = any local machine address
	serv_addr.sin_port = htons(atoi(argv[2]));

	if (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))){
		printf("\n Error : Bind Failed. %s \n", strerror(errno));
		return 1;
	}

	if (listen(listenfd, 10)){
		printf("\n Error : Listen Failed. %s \n", strerror(errno));
		return 1;
	}

	while (!isDone) {

		connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
		printf("hrtr\n");
			
		if (isDone) break;

		if (connfd<0){
			printf("\n Error : Accept Failed. %s \n", strerror(errno));
		}

		enqueue(initNode(connfd));

	}

	for (i = 0; i <numOfThreads; i++) {
		rc = pthread_mutex_lock(&sharedMutex); assert(!rc);
		pthread_cond_signal(&notEmptyCond);
		rc = pthread_mutex_unlock(&sharedMutex); assert(!rc);
	}

	sleep(2); // sleep a while to make sure everybody is gone

	// clean up allocated pthread structs
	printf("Clean up\n");
	pthread_mutex_destroy(&sharedMutex);
	pthread_cond_destroy(&notEmptyCond);
	free(thread);
	setos_free();
	printf("Main completed\n");

	return 0;
}

