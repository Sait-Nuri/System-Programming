#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <stdlib.h>
#include <fcntl.h>

#define SHMKEY 1234
#define SM_NAME1 "$HOME/sistem/sema1"
#define SM_NAME2 "$HOME/sistem/sema2"
 
main()
{
	int shmid;
	key_t key;
	char *shm;
	sem_t *sem1;
	sem_t *sem2; 	

 	if ((sem1 = sem_open(SM_NAME1, O_CREAT)) == (sem_t *) -1) {
		perror("sem1:");
		exit(1);
	}

	if ((sem2 = sem_open(SM_NAME2, O_CREAT)) == (sem_t *) -1) {
		perror("sem2:");
		exit(1);
	}

	/*
	* Locate the segment.
	*/
	if ((shmid = shmget(key, sizeof(int), IPC_CREAT|0666)) < 0) {
		perror("shmget");
		return 1;
	}
 
	/*
	* Now we attach the segment to our data space.
	*/
	if ((shm = shmat(shmid, NULL, 0)) == (char *) -1) {
		perror("shmat");
		return 1;
	}
 
	/*
	* Zero out memory segment
	*/
	memset(shm,0,sizeof(int));
 
	/*
	* Client writes user input character to memory
	* for server to read.
	*/
	for(;;){
		printf("waiting...\n");
		sem_wait(sem2);
		printf("entered...\n");
		char tmp = getchar();
		// Eat the enter key
		getchar();
 
		if(tmp == 'q'){
			*shm = 'q';
			break;
		}
		*shm = tmp;
		sem_post(sem1);
	}
 
	if(shmdt(shm) != 0)
		fprintf(stderr, "Could not close memory segment.\n");
 
	return 0;
}