#include <sys/stat.h>
#include <string.h>
#include <termios.h>
#include <errno.h>
#include <termios.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>

#define MAX_LENGHT 512
#define SHMKEY 1234
#define SHMMSGKEY 4321
#define SHMINDEX 1111
#define SM_NAME1 "sema1"
#define SM_NAME2 "sema2"
#define SM_CLIENT "client"
#define THOUSAND 1000
#define BILLION 1000000L

struct timespec start;	

double get_avarage(double*, int);
void reporting(double , FILE* );
void print_usage_server();
void print_segment(double*, int);

/* flag for signal handling */
volatile sig_atomic_t die_flag = 0;

/* This static function to handle specified signal */
static void signal_func(int signo) {
    
    /* The signal flag */    
    printf("singal has caught! Please wait....\n");    
    die_flag = 1;
    
}

main(int argc, char const *argv[])
{
	int shmid, shm_msg_id, shm_index;
	int buffer_size;
	int sampling_time;
	int size;
	int *message;
	int i;
	int* index;
	int temp;
	double* wheel;
	double avarage;
	char *shm;
 	sem_t *sem1;
	sem_t *sem2;
	FILE * logfile;		

	if(argc != 3){
		printf("invalid operation!\n");
		print_usage_server();
	}

	sem_unlink(SM_NAME1);
	sem_unlink(SM_NAME2);

	/* initialize logfile */	
	if( (logfile = fopen("mserver_log", "w")) < 0){
		perror("logfile:");
		exit(EXIT_FAILURE);
	}

	/* Signal handling */
    if (signal(SIGINT, signal_func) == SIG_ERR) {
        fputs("An error occurred while setting a signal handler.\n", stderr);
        exit( EXIT_FAILURE);
   	}

	clock_gettime(CLOCK_REALTIME, &start);  
	fprintf(logfile, "Server started at %ld sec.\n", (start.tv_sec + start.tv_nsec/BILLION) );
	fprintf(logfile, "Avarage\t\t\t\t\tTime\n");
	fprintf(logfile, "-------\t\t\t\t------------\n");

	/* initialize the buffer size */
	buffer_size = atoi(argv[1]);	

	/* initialize the sampling time */
	sampling_time = atoi(argv[2]);	

    /************************** Semaphore and Shared memory *************************************/
    /* Protection for shared memory */
    if ((sem1 = sem_open(SM_NAME1, O_CREAT, 0666, 1)) < 0) {
		perror("sem1:");
		exit(1);
	}
	
	if ((sem2 = sem_open(SM_NAME2, O_CREAT, 0666, 0)) < 0) {
		perror("sem2:");
		exit(1);
	}
	
	if ((shmid = shmget(SHMKEY, buffer_size, IPC_CREAT | 0666)) < 0) {
		perror("shmget");
		return 1;
	}
 	
	if ((shm = shmat(shmid, NULL, 0)) < 0) {
		perror("shmat");
		return 1;
	}

 	if ((shm_msg_id = shmget(SHMMSGKEY, sizeof(int), IPC_CREAT | 0666)) < 0){
 		perror("shmget");
		return 1;	
 	}

 	if ((message = shmat(shm_msg_id, NULL, 0)) < 0) {
		perror("shmat");
		return 1;
	}

	if ((shm_index = shmget(SHMINDEX, sizeof(int), IPC_CREAT | 0666)) < 0){
 		perror("shmget");
		return 1;	
 	}

 	if ((index = shmat(shm_index, NULL, 0)) < 0) {
		perror("shmat");
		return 1;
	}
	/***************************************************************************/

	/* Casting wheel of fortune to double array */
	wheel = (double*)shm;
	
	/* Determine the size of double array */
	size = (buffer_size / sizeof(double));	

	/************** -Send the size- ******************/	
	*message = size;
	// printf("size: %d\n", *message);		
	
	/***********************************************/
	
	/******************* -initialize wheel- *******************/
	for(i = 0; i < size; i++){		
		wheel[i] = (1.0/size)*i;
	}
	/***********************************************/

	/**************** -Avarage Part- ************************/	
	while(!die_flag){		
		usleep(sampling_time*THOUSAND);	
		sem_wait(sem1);						//printf("in critical sem1: %d\n", (int)sem1->__align);			
		avarage = get_avarage(wheel, size);	
		printf("avarage: %.2f\n", avarage);	
		// print_segment(wheel, size);	
		srand(time(NULL));
		*index = rand() % size;
		reporting( avarage, logfile );
		sem_post(sem1);						//printf("out of critical sem1: %d\n\n", (int)sem1->__align);
	}		
	/*************************************************/
	
	if(die_flag){
		//printf("mesaj görüldü\n");
		fprintf(logfile, "Server terminated by user\n" );
		*message = 0;
	}

	if(shmdt(shm) != 0)
		fprintf(stderr, "Could not close memory segment.\n");
	
	if(shmdt(message) != 0)
		fprintf(stderr, "Could not close memory segment.\n");
	
	if(shmdt(index) != 0)
		fprintf(stderr, "Could not close memory segment.\n");
	 
 	sem_close(sem1);
 	sem_close(sem2);
 	fclose(logfile);

	return 0;
}
/*
	Gets the avarege of the values in wheel of fortune
	@param wheel A shared memory for wheel of fortune keeps double values
	@param size Size of the wheel of fortune
	@return Avarage of values of the wheel of fortune
*/
double get_avarage(double* wheel, int size){
	double sum = 0;
	int i;

	for (i = 0; i < size; ++i){
		sum += wheel[i];
	}

	// printf("avarage: %f\n", sum/size);
	return sum/size;
}

void reporting(double avarage, FILE* logfile){
	struct timespec current;
	clock_gettime(CLOCK_REALTIME, &current);   
	fprintf(logfile, "%.2f\t\t\t\t%ld millisec.\n", avarage, ( (current.tv_sec-start.tv_sec)*THOUSAND + 
		(current.tv_nsec/THOUSAND-start.tv_nsec/THOUSAND) ));
}

void print_usage_server(){
	printf("Usage: ./mserver <buffer_size> <update_time [millisec]>\n");
	printf("Parsing standart: (<function>)^(<power>)\n");
	printf("Example: (x^2)^(1/3)\n");
	exit(-1);
}
void print_segment(double* segment, int size){
	int i;

	for(i = 0; i < size; i++){			
		printf("%0.2f\n", segment[i]);	
	}
}