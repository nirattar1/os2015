#include <stdio.h>
#include <assert.h>
#include <pthread.h>

void *t1(void *arg) 
{
    printf("A\n");
    return NULL;
}

void *t2(void *arg) 
{
    printf("B\n");
    return NULL;
}

int main(int argc, char *argv[])
{
    pthread_t p1, p2;
    int rc1, rc2;

    printf("main: begin\n");
    rc = pthread_create(&p1, NULL, t1, NULL);
    rc = pthread_create(&p2, NULL, t2, NULL);
	
	assert(rc1 == 0);
	assert(rc2 == 0);

    // join waits for the threads to finish
    rc1 = pthread_join(p2, NULL);
    rc2 = pthread_join(p1, NULL);
	
	assert(rc1 == 0);
	assert(rc2 == 0);

    printf("main: done with both\n");
    return 0;
}
