/* Includes */
#include <unistd.h>     /* Symbolic Constants */
#include <sys/types.h>  /* Primitive System Data Types */ 
#include <errno.h>      /* Errors */
#include <stdio.h>      /* Input/Output */
#include <stdlib.h>     /* General Utilities */
#include <pthread.h>    /* POSIX Threads */
#include <string.h>     /* String handling */
#include <semaphore.h>  /* Semaphore */

#define THREAD_NUMBER 10

/* prototype for thread routine */
void handler ( void *ptr );

/* global vars */
sem_t mutex;

int main()
{
    pthread_t threads[THREAD_NUMBER];
    int counter = 0; /* shared variable */
    int i;

    sem_init(&mutex, 0, 1);      /* initialize mutex to 1 - binary semaphore */
                                 /* second param = 0 - semaphore is local */
            
    for (i = 0; i < THREAD_NUMBER; ++i){
        pthread_create (&threads[i], NULL, (void *) &handler, (void *) &counter);
    }                            

    for (i = 0; i < THREAD_NUMBER; ++i){
        pthread_join(threads[i], NULL);
    }

    sem_destroy(&mutex); /* destroy semaphore */
                  
    /* exit */  
    exit(0);
} /* main() */

void handler ( void *ptr )
{
    int * counter; 
    counter = ((int *) ptr);
    printf("Thread %d: Waiting to enter critical region...\n", (unsigned int)pthread_self());
    sem_wait(&mutex);       /* down semaphore */
    /* START CRITICAL REGION */
    sleep(3);
    printf("Thread %d: Now in critical region...\n", (unsigned int)pthread_self());
    (*counter)++;
    printf("Thread %d: New Counter Value: %d\n", (unsigned int)pthread_self(), *counter);
    printf("Thread %d: Exiting critical region...\n", (unsigned int)pthread_self());
    /* END CRITICAL REGION */    
    sem_post(&mutex);       /* up semaphore */
    
    pthread_exit(0); /* exit thread */
}