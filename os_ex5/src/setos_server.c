/*
 * setos_server.c
 *
 *  Created on: Jun 18, 2015
 *      Author: nirattar
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <assert.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include "setos.h"



//lock on queue
pthread_mutex_t _global_lock;

//condition (empty queue)
pthread_cond_t  _cond_empty;


//globals

int	NUM_THREADS = 0;
int	status = 0;
struct _linked_list* _list;
//for cond. variable
int	queue_empty = 1;
int	done = 0;


//////////////
// node struct
//////////////

typedef struct _list_node
{
	int	data;
	struct _list_node*	next;
	struct _list_node*	prev;

} list_node;

//////////////
// list struct
//////////////

typedef struct _linked_list{
	int		num_elems;
	list_node*	_head;
	list_node*	_tail;

} linked_list;

//////////////


//handle signal
static void sigHandle(int signal_type)
{
	if (signal_type == SIGINT)
	{
		done = 1;
	}
}





int list_init()
{
	_list = (linked_list*)malloc(sizeof(linked_list));
	_list->_head = NULL;
	_list->_tail= NULL;

	if (status)
	{
		printf("err: Failed to init list. \n");
		return(1);
	}
	return 0;
}

list_node * InitNode(int skfd)
{
	list_node  * newNode		= (list_node*) malloc(sizeof(list_node));
	newNode->data			= skfd;
	newNode->next			= NULL;
	newNode->prev			= NULL;
	return newNode;
}

///////////////
//retrieve a socket from the queue
list_node* Dequeue()
{
	list_node* node = NULL;
	status = pthread_mutex_lock(&_global_lock);

	while ( queue_empty && !done)
	{

		status = pthread_cond_wait(&_cond_empty, &_global_lock); //wait until not empty

		if (status)
		{
			pthread_mutex_unlock(&_global_lock);
			exit(1);
		}
	}


	//condition for waking signal after done == 1
	if (!queue_empty)
	{
		assert(_list!=0 && _list->_tail!=0);
		node = _list->_tail;
		_list->_tail = node->prev;
		if (_list->_tail)
		{
			_list->_tail->next = NULL;
		}
		_list->num_elems--;
		queue_empty = (_list->num_elems == 0) ? 1 : 0;
	}

	status = pthread_mutex_unlock(&_global_lock);

	return node;

}

///////////////
//adds a socket into the queue
int Enqueue(list_node* node)
{
	status = pthread_mutex_lock(&_global_lock);

	if (_list->num_elems == 0)
	{
		_list->_head = node;
		_list->_tail = node;
		_list->num_elems++;
		queue_empty = 0;
	}
	else
	{
		list_node* prev = _list->_tail;
		prev->next = node;
		_list->_tail = node;
		node->prev = prev;
		_list->num_elems++;
	}

	//send signal (for threads trying to deque)
	status = pthread_cond_signal(&_cond_empty);
	status = pthread_mutex_unlock(&_global_lock);


	return 0;
}



//will read from socket
void * DoWork(void * param)

{
	char recvBuff[5];
	int need_read = sizeof(recvBuff);
	int totalRead = 0;
	int nread = 0;
	int closed = 0;
	int res;

	list_node * node;
	int sock_fd;



	while (!queue_empty || !done )
	{

		node = Dequeue();

		while (!closed && node!= NULL)
		{
			sock_fd = node->data;

			//repeat until finished reading
			while (need_read > 0)
			{

				nread = read(sock_fd, recvBuff + totalRead, need_read);


				if (nread < 1)
				{
					close(sock_fd);
					closed = 1;
				}

				//update how much we've read
				totalRead += nread;
				need_read -= nread;
			}

			//have full data- can call appropriate command.
			if (totalRead == 5)
			{

				if (atoi(recvBuff) == 0)
				{
					res = setos_add(ntohl(*((uint32_t *)(recvBuff + 1))),NULL);
				}
				else if (atoi(recvBuff) == 1)
				{
					res = setos_remove(ntohl(*((uint32_t *)(recvBuff + 1))), NULL);
				}
				else
				{
					res = setos_contains(ntohl(*((uint32_t *)(recvBuff + 1))));
				}

				//reply to client
				write(sock_fd, &res, 1);


			}


			else
			{
				printf("err: thread %u. Failed to read.\n", (unsigned int)pthread_self());
			}
		}

		closed = 0;
	}


	return NULL;
}



int main(int argc, char **argv)
{

	int	 NUM_THREADS = atoi(argv[1]);
	struct sockaddr_in	serv_addr;

	int	i;
	int	listenfd = 0;
	int	connfd = 0;
	pthread_t* thread;
	struct sigaction sa;

	//signal handling
	sa.sa_handler = sigHandle;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGPIPE, &sa, NULL);



	//init threads
	thread = (pthread_t*) malloc ( NUM_THREADS * sizeof(pthread_t) );

	//init lock and cond variable
	status = pthread_mutex_init(&_global_lock ,NULL);
	status = pthread_cond_init(&_cond_empty, NULL);

	//socket list
	list_init();

	//init of "set" data structure
	setos_init();

	for (i = 0; i <NUM_THREADS; i++)
	{
		status = pthread_create(&thread[i], NULL, DoWork, NULL);
	}

	sleep(2);

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	memset(&serv_addr, '0', sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(atoi(argv[2]));


	//bind to port
	if (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))){
		printf("\nerr: failed bind %s \n", strerror(errno));
		return 1;
	}

	//listen on port
	if (listen(listenfd, 10)){
		printf("\nerr: failed listen %s \n", strerror(errno));
		return 1;
	}

	while (!done)
	{

		connfd = accept(listenfd, (struct sockaddr*) NULL, NULL);

		if (done) break;

		if (connfd<0)
		{
			printf("\nerr: failed accept %s \n", strerror(errno));
		}

		Enqueue(InitNode(connfd));

	}

	for (i = 0; i <NUM_THREADS; i++) {
		status = pthread_mutex_lock(&_global_lock); assert(!status);
		pthread_cond_signal(&_cond_empty);
		status = pthread_mutex_unlock(&_global_lock); assert(!status);
	}

	sleep(2);


	// clean up allocated threads, if exist
	pthread_mutex_destroy(&_global_lock);
	pthread_cond_destroy(&_cond_empty);
	free(thread);
	setos_free();

	return 0;
}


