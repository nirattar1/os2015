#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>

/************* globals **************/  
#define NUMTHREADS     2

// mutex and cond variable
pthread_mutex_t sharedElementMutex; // mutex for access to shared "data"
pthread_cond_t  canConsumeCondition; // condition variable to wake up "consumers" of data

// our shared predicates and "data"
int canConsume = 0; // used to signal consumers waiting on condition variable
int sharedElement  = 0; // "data" shared with cnsumers
int done = 0; // used to signal consumers they should exit gracefuly
/************************************/  

void *theThread(void *parm){
   int rc;
 
   printf("Consumer Thread %u: Entered\n", pthread_self());

   // initial lock
   rc = pthread_mutex_lock(&sharedElementMutex); assert(!rc);
 
   // main loop. verifies producer did not signal us to terminate
   // and "processes" any "data"
   while (!done) {
      /* secondary loop, for condition variable
       * canConsume required for safe use of condition variables. 
       * no data - wait. otherwise - process immediately */
      while (!canConsume) {
         printf("Consumer Thread %u: Wait for data to be produced\n", pthread_self());
         // note - mutex is already locked here!
         rc = pthread_cond_wait(&canConsumeCondition, &sharedElementMutex);
         // mutex locked here too!
         if (rc) {
            printf("Consumer Thread %u: condwait failed, rc=%d\n", pthread_self(), rc);
            pthread_mutex_unlock(&sharedElementMutex);
            exit(1);
         }

         if(done){
             break;
         }
      }

      /* producer thread is done  */
      if(done){ 
	      printf("Consumer thread %u: signaled to exit gracefuly. exit loop\n", pthread_self());
          break;
      }

	  printf("Consumer thread %u: Found data! hooray\n", pthread_self());
      /* remove the shared data */
      sharedElement--;

      /* We consumed the last of the data */
      if (sharedElement==0){
          canConsume=0;
      }

      /* repeat while still holding the lock. pthread_cond_wait releases it atomically */
   }

   printf("Consumer Thread %u: All done\n",pthread_self());
   rc = pthread_mutex_unlock(&sharedElementMutex);

   assert(!rc);
   return NULL; // exiting gracefuly
}
 
int main(int argc, char **argv){
   pthread_t thread[NUMTHREADS];
   int rc = 0;
   int elementsToConsume = 4; // all the data that will be consumed
   int i;
 
   /* init mutex and cond variable */
   rc = pthread_mutex_init(&sharedElementMutex, NULL); assert(rc==0);
   rc = pthread_cond_init(&canConsumeCondition, NULL); assert(rc==0);

   printf("Create/start threads\n");
   for (i=0; i <NUMTHREADS; i++) {
      rc = pthread_create(&thread[i], NULL, theThread, NULL); assert(!rc);
   }
   sleep(2); // give time for all consumers to start

   /* The producer loop. make one "element" shared, so consumers can access it */
   while (elementsToConsume--) {         
      printf("Producer: 'Get' data\n");
	  
	  /* Protect shared data and flag  */
      rc = pthread_mutex_lock(&sharedElementMutex); assert(!rc);
      printf("Producer: Make element shared and notify consumer\n");
      sharedElement++; /* Add data */
      canConsume=1; /* Set boolean predicate         */
 
      rc = pthread_cond_signal(&canConsumeCondition); /* wake up a consumer  */
      if (rc) {
         pthread_mutex_unlock(&sharedElementMutex);
         printf("Producer: Failed to wake up consumer, rc=%d. Exiting\n", rc);
         exit(1);
      }
 
      printf("Producer: Unlock shared data and flag (elementsToConsume=%d)\n", elementsToConsume);
      rc = pthread_mutex_unlock(&sharedElementMutex); assert(!rc);
      sleep(1);
   }

   printf("Wait for the threads to complete, and release their resources\n");
   sleep(1); // let other threads finish consuming
   rc = pthread_mutex_lock(&sharedElementMutex); assert(!rc);
   done = 1;
   rc = pthread_mutex_unlock(&sharedElementMutex); assert(!rc);

   for (i=0; i <NUMTHREADS; i++) {
       rc = pthread_mutex_lock(&sharedElementMutex); assert(!rc);
       pthread_cond_signal(&canConsumeCondition);
       rc = pthread_mutex_unlock(&sharedElementMutex); assert(!rc);
   }
   
   sleep(2); // sleep a while to make sure everybody is gone

   // clean up allocated pthread structs
   printf("Clean up\n");
   pthread_mutex_destroy(&sharedElementMutex);
   pthread_cond_destroy(&canConsumeCondition);
   printf("Main completed\n");
   return 0;
}
