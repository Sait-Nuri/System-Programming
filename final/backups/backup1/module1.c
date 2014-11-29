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
#define BUFFER_NUMBER 100; 

typedef struct args{
	int matrix_size;
	int nthread;
}arg_t;

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
int inversTaker();

pthread_mutex_t decompose_thread_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t matrix_thread_lock = PTHREAD_MUTEX_INITIALIZER;
matrix_t** matrix_buffer ;
int matrix_index = 0;

/* flag for signal handling */
volatile sig_atomic_t die_flag = 0;

/* This static function to handle specified signal */
static void signal_func(int signo) {
    /* The signal flag */    
    die_flag = 1;
    printf("singal has caught! Please wait....\n");    
}

int main(int argc, char const *argv[]){
	
	int size_of_matrices;
	int numberof_threads;
	int numberof_buffer = BUFFER_NUMBER;
	pthread_t rmp_thread, decom_thread, invtak_thread;
	arg_t args;

	/* Command line argumant control */
	if(argc < 3 || argc > 3){
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
	if( (size_of_matrices = atoi(argv[1])) > 40){
		printf("matrix size cannot be larger than 40 \n");
		exit(EXIT_FAILURE);
	}
	printf("size_of_matrices: %d\n", size_of_matrices);

	if(size_of_matrices < 1){
		printf("Invalid matrix size! %d \n", size_of_matrices);
		exit(EXIT_FAILURE);
	}

	/* Control of number of threads*/
	if((numberof_threads = atoi(argv[2])) < 1){
		printf("Invalid number of threads!\n");
		exit(EXIT_FAILURE);	
	}

	printf("numberof_threads %d\n", numberof_threads);

	args.matrix_size = size_of_matrices;
	args.nthread = numberof_threads;

	matrix_buffer = (matrix_t**) calloc(numberof_buffer, sizeof(matrix_t*));

	/* Create threads for each module */
	pthread_create(&rmp_thread, NULL, (void*) &randomMatrixProducer, &args);
	pthread_create(&decom_thread, NULL, (void*) &l_u_decomposer, NULL);
	pthread_create(&invtak_thread, NULL, (void*) &inversTaker, NULL);

	pthread_join(rmp_thread, NULL);
	pthread_join(decom_thread, NULL);
	pthread_join(invtak_thread, NULL);

	return 0;
}

int randomMatrixProducer(void* arg){	
	
	int i;
	int numberof_threads = (*(arg_t*)arg).nthread;
	int size_of_matrices = (*(arg_t*)arg).matrix_size;
	pthread_t *threads;	

	/* Allocation of threads */
	threads = (pthread_t*) calloc(numberof_threads, sizeof(pthread_t));

	/* Thread creation */
	for (i = 0; i < numberof_threads; ++i){
        pthread_create(&threads[i], NULL, (void *) &produce_matrix, &size_of_matrices);
    }	

    // /* Wait for thread to be killed */
    for(i = 0; i < numberof_threads; ++i){
    	pthread_join(threads[i], NULL);
    }    

    free(threads);

	return 0;
}

void produce_matrix(void* size){
	int matrix_size = *(int*)size;
	int error;
	float matrixL[matrix_size][matrix_size],
		  matrixU[matrix_size][matrix_size];
	int i,j;	

	while(!die_flag){
		printf("thread: %u waits for lock \n", (unsigned int) pthread_self());

		if(error = pthread_mutex_lock(&matrix_thread_lock) < 0){
			perror("matrix_thread_lock");
			exit(EXIT_FAILURE);
		}
		printf("thread: %u in critical section \n", (unsigned int) pthread_self());
	
		srand(time(NULL));
		for(i = 0; i < matrix_size; ++i){
			for (j = 0; j < matrix_size; ++j){								
				matrixL[i][j] = matrixU[i][j] = abs((rand())%50);
			}
		}
		
	    for(i = 0; i < matrix_size; i++){
			for(j = 0; j < matrix_size; j++)
				printf("%6.2f ", matrixL[i][j]);
			printf("\n");
		}

		if(error = pthread_mutex_unlock(&matrix_thread_lock) < 0){
			perror("matrix_thread_lock");
			exit(EXIT_FAILURE);
		}
	}
	printf("thread bitti\n");
}

void print_usage(){
	puts("Usage: ./module1 <size of matrices> <# of threads>");
	puts("Example: ./module1 20 5");
}
int l_u_decomposer(){
	pthread_t
}
int inversTaker(){

}