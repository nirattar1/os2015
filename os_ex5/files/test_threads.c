

void *theThread(void *parm){
   int rc, x=1;

   printf("Thread %u: Entered, with param: %d \n", pthread_self(), *((int*)parm));
   x=num;
   setos_add(num++, &x);
   setos_contains (x);
   setos_remove(x,NULL);
   printf("Thread %u: Ended, with param: %d \n", pthread_self(), *((int*)parm));
   return NULL; // exiting gracefuly
}
int main(void)
{
	int rc,x = 1;
	puts ("init\n");
	setos_init();

	pthread_t thread[NUMTHREADS];


  	 int i;

  	 /* init mutex and cond variable */
  	 rc = pthread_mutex_init(&sharedElementMutex, NULL); assert(rc==0);
  	 //rc = pthread_cond_init(&canConsumeCondition, NULL); assert(rc==0);

  	 printf("Create/start threads\n");
  	 for (i=0; i <NUMTHREADS; i++) {
	printf("%d\n", i);
   	   rc = pthread_create(&thread[i], NULL, theThread, &i); assert(!rc);
  	 }
   	sleep(3); // give time for all consumers to start

	/*
	puts ("add 1\n");
	rc= setos_add(1, &x);
	printf ("addition result:  %d\n ", rc);

	//printf ("%d", pthread_mutex_trylock (&sharedElementMutex));
	puts ("add 2");
	setos_add(2, &x);
	puts ("add 3");
	setos_add(3, &x);
*/
	puts("");
	puts ("contains");
	assert(setos_contains(1));

	assert(!setos_contains(4));
	/*puts ("remove");
	assert(!setos_remove(4,NULL));
	assert(setos_remove(1,NULL));
	puts ("contains 2");
	assert(!setos_contains(2));
*/
	puts ("free");
	setos_free();
	printf ("end");
	return 0;
}

