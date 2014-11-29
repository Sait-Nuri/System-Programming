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

main(int argc, char **argv)
{
	int shmid;
	key_t key;
	char *shm;
 	sem_t *sem1;
	sem_t *sem2;
    
    if ((sem1 = sem_open(SM_NAME1, (O_CREAT), 0666, 1)) == (sem_t *) -1) {
		perror("sem1:");
		exit(1);
	}

	if ((sem2 = sem_open(SM_NAME2, (O_CREAT), 0666, 1)) == (sem_t *) -1) {
		perror("sem2:");
		exit(1);
	}
    /*
     * Create the segment and set permissions.
     */
	if ((shmid = shmget(SHMKEY, sizeof(int), IPC_CREAT | 0666)) < 0) {
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

	while (*shm != 'q'){
		sleep(1);
		printf("waiting...\n");
		sem_wait(sem1);
		printf("entered...\n");

		/* Critical section */
		fprintf(stdout, "You pressed %c",*shm);
		sem_post(sem2);
	}

	if(shmdt(shm) != 0)
		fprintf(stderr, "Could not close memory segment.\n");
 
 	sem_close(sem1);
 	sem_close(sem2);

	return 0;
}