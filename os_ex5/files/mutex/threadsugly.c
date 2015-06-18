#include <stdio.h>
#include <pthread.h>
#include <assert.h>

static int balance = 0;

#define NUM 100000000

void *mythread(void *arg)
{
    char *letter = arg;

    int i;
    printf("%s: begin\n", letter);
    for (i = 0; i < NUM; i++) {
        balance = balance + 1;
    }
    printf("%s: done\n", letter);
    return NULL;
}

int main(int argc, char *argv[])
{
    int rc;
    pthread_t p1, p2;
    printf("main: begin [balance = %d]\n", balance);
    rc = pthread_create(&p1, NULL, mythread, "A"); assert(rc == 0);
    rc = pthread_create(&p2, NULL, mythread, "B"); assert(rc == 0);

    // join waits for the threads to finish
    rc = pthread_join(p1, NULL); assert(rc == 0);
    rc = pthread_join(p2, NULL); assert(rc == 0);
    printf("main: done with both [balance = %d]\n", balance);
    return 0;
}
