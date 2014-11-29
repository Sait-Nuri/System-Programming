#include <stdio.h>
#include <pthread.h> 
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>

typedef struct matrix{
	float** m;
}matrix_t;

typedef struct lu{
	float** l;
	float** u;
}lu_t;

void print_usage();
void produce_matrix(void*);
int randomMatrixProducer(void*);
int l_u_decomposer();
void decompose(void* arg);
void decomposing(float**, float**);
int inversTaker();

pthread_mutex_t decompose_thread_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t matrix_thread_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t prod_decom_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t inverser_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t decom_inv_lock = PTHREAD_MUTEX_INITIALIZER;

matrix_t *matrix_queue;
lu_t *lu_buffer;

int mque_index = -1;
int luque_index = -1;

int numberof_threads;
int MATRIX_SIZE;
int matrix_count = 0;
int maximum_produce;
int QUEUE_SIZE;

/* flag for signal handling */
volatile sig_atomic_t die_flag = 0;

/* This static function to handle specified signal */
static void signal_func(int signo) {
    /* The signal flag */    
    die_flag = 1;
    printf("singal has caught! Please wait....\n");    
}

int main(int argc, char const *argv[]){
	
	int i,j,k;
	pthread_t rmp_thread, decom_thread, invtak_thread;

	/* Command line argumant control */
	if(argc < 5 || argc > 5){
		puts("Invalid operation!");
		print_usage();
		exit(EXIT_FAILURE);
	}

	/* Signal handling */
    if (signal(SIGINT, signal_func) == SIG_ERR) {
        fputs("An error occurred while setting a signal handler.\n", stderr);
        exit( EXIT_FAILURE);
   	}

	/* Matrix size control */
	if( (MATRIX_SIZE = atoi(argv[1])) > 40){
		printf("matrix size cannot be larger than 40 \n");
		exit(EXIT_FAILURE);
	}
	printf("MATRIX_SIZE: %d\n", MATRIX_SIZE);

	if(MATRIX_SIZE < 1){
		printf("Invalid matrix size! %d \n", MATRIX_SIZE);
		exit(EXIT_FAILURE);
	}

	/* Control of number of threads*/
	if((numberof_threads = atoi(argv[2])) < 1){
		printf("Invalid number of threads!\n");
		exit(EXIT_FAILURE);	
	}
	printf("numberof_threads %d\n", numberof_threads);

	if((maximum_produce = atoi(argv[3])) < 1){
		printf("Invalid number of maximum_produce!\n");
		exit(EXIT_FAILURE);	
	}	
	printf("maximum_produce %d\n", maximum_produce);
	
	if((QUEUE_SIZE = atoi(argv[4])) < 1){
		printf("Invalid QUEUE_SIZE!\n");
		exit(EXIT_FAILURE);	
	}	
	printf("QUEUE_SIZE %d\n", QUEUE_SIZE);

	matrix_queue = calloc(QUEUE_SIZE, sizeof(matrix_t));
	lu_buffer = calloc(QUEUE_SIZE, sizeof(lu_t));

	srand(time(NULL));

	/* Create threads for each module */
	pthread_create(&rmp_thread, NULL, (void*) &randomMatrixProducer, &maximum_produce);
	pthread_create(&decom_thread, NULL, (void*) &l_u_decomposer, NULL);
	pthread_create(&invtak_thread, NULL, (void*) &inversTaker, NULL);

	pthread_join(rmp_thread, NULL);
	pthread_join(decom_thread, NULL);
	pthread_join(invtak_thread, NULL);

	// for(i = 0; i <= mque_index; ++i){
	// 	for (j = 0; j < MATRIX_SIZE; ++j)	{
	// 		free(matrix_queue[i].m[j]);
	// 	}
	// 	free(matrix_queue[i].m);		
	// }

	// for(i = 0; i <= luque_index; ++i){
	// 	for (j = 0; j < MATRIX_SIZE; ++j){
	// 		free(lu_buffer[i].l[j]);
	// 		free(lu_buffer[i].u[j]);
	// 	}
	// 	free(lu_buffer[i].l);		
	// 	free(lu_buffer[i].u);		
	// }

	return 0;
}

int randomMatrixProducer(void* arg){	
	
	int i;
	int nproduce = *(int *)arg;
	pthread_t *threads;	

	/* Allocation of threads */
	threads = (pthread_t*) calloc(numberof_threads, sizeof(pthread_t));

	/* Thread creation */
	for (i = 0; i < numberof_threads; ++i){
        pthread_create(&threads[i], NULL, (void *) &produce_matrix, arg);
    }	

    // /* Wait for thread to be killed */
    for(i = 0; i < numberof_threads; ++i){
    	pthread_join(threads[i], NULL);
    }    
    free(threads);
    printf("randomMatrixProducer\n");
	return 0;
}

void produce_matrix(void* size){

	while(!die_flag){

		/* Mutex for matrices race condition */
		if(pthread_mutex_lock(&matrix_thread_lock) < 0){
			continue;
		}

		/* If produced matrix size exceeded the max produce, release lock and get out of here */
		if(matrix_count >= MAX_PRODUCE){
			printf("max size reached %d\n", matrix_count);
			pthread_mutex_unlock(&matrix_thread_lock)
			break;
		}

		/******************* -Produce- *************************/
		/* If matrix queue index reached max queue size, release the lock and try again */
		if(mque_index >= QUEUE_SIZE){
			if(pthread_mutex_unlock(&matrix_thread_lock) < 0){
				perror("matrix_thread_unlock");
				exit(EXIT_FAILURE);
			}
			printf("mque_index higher than %d!. luque_index %d\n", QUEUE_SIZE, luque_index);
			continue;
		}

		/* Capture the lock for produce matrix */
		if(pthread_mutex_lock(&prod_decom_lock) < 0){
			perror("prod_decom_lock");
			exit(EXIT_FAILURE);
		}

		/* increase number of produced matrix */
		++matrix_count;

		/* Shift the queue */
		if(mque_index != -1){
			for(i = mque_index; i >= 0; ++i){
				matrix_queue[i+1].m = matrix_queue[i].m;
			}	
		}
		
		/* Increase the index of matrix queue */
		++mque_index;
		printf("mque_index: %d\n", mque_index);

		/****** Allocation for matrix ****/
		matrix_queue[mque_index].m = calloc(MATRIX_SIZE, sizeof(float*));
		
		for(i = 0; i < MATRIX_SIZE; ++i){
			matrix_queue[mque_index].m[i] = calloc(MATRIX_SIZE, sizeof(float));
		}
		/*********************************/

		/* Generate matrix */
		for(i = 0; i < MATRIX_SIZE; ++i){
			for (j = 0; j < MATRIX_SIZE; ++j){								
				matrix_queue[mque_index].m[i][j] = abs((rand())%50);
			}
		}

		/* Release the prod_decom_lock */
		if(pthread_mutex_unlock(&prod_decom_lock) < 0){
			perror("prod_decom_unlock");
			exit(EXIT_FAILURE);
		}
		/***********-END PRODUCE- *************/

		/* Release matrix_thread_lock between threads */
		if(error = pthread_mutex_unlock(&matrix_thread_lock) < 0){
			perror("matrix_thread_lock");
			exit(EXIT_FAILURE);
		}		
	}
	printf("End of thread\n");
}