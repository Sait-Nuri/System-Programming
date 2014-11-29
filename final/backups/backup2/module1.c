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

#define BUF_NUM 100; 

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
int inversTaker();

pthread_mutex_t decompose_thread_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t matrix_thread_lock = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t prod_decom_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t produce = PTHREAD_COND_INITIALIZER;
pthread_cond_t consume = PTHREAD_COND_INITIALIZER;

matrix_t** matrix_buffer ;
lu_t** lu_buffer;

int mbuf_index = 0;
int lu_buf_index = 0;
int numberof_threads;
int size_of_matrices;

/* flag for signal handling */
volatile sig_atomic_t die_flag = 0;

/* This static function to handle specified signal */
static void signal_func(int signo) {
    /* The signal flag */    
    die_flag = 1;
    printf("singal has caught! Please wait....\n");    
}

int main(int argc, char const *argv[]){
	

	int numberof_buffer = BUF_NUM;
	int i,j,k;
	pthread_t rmp_thread, decom_thread, invtak_thread;

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
	matrix_buffer = (matrix_t**) calloc(numberof_buffer, sizeof(matrix_t*));
	lu_buffer = (lu_t**)calloc(numberof_buffer, sizeof(lu_t*));

	/* Create threads for each module */
	pthread_create(&rmp_thread, NULL, (void*) &randomMatrixProducer, NULL);
	pthread_create(&decom_thread, NULL, (void*) &l_u_decomposer, NULL);
	pthread_create(&invtak_thread, NULL, (void*) &inversTaker, NULL);

	pthread_join(rmp_thread, NULL);
	pthread_join(decom_thread, NULL);
	pthread_join(invtak_thread, NULL);

	for(i = 0; i < mbuf_index; ++i){
		for (j = 0; j < size_of_matrices; ++j)	{
			free(matrix_buffer[i]->m[j]);
		}
		free(matrix_buffer[i]->m);		
		free(matrix_buffer[i]);
	}
	free(matrix_buffer);

	for(i = 0; i < lu_buf_index; ++i){
		for (j = 0; j < size_of_matrices; ++j)	{
			free(lu_buffer[i]->l[j]);
			free(lu_buffer[i]->u[j]);
		}
		free(lu_buffer[i]->l);		
		free(lu_buffer[i]->u);		
		free(lu_buffer[i]);
	}
	free(lu_buffer);

	return 0;
}

int randomMatrixProducer(void* arg){	
	
	int i;
	pthread_t *threads;	

	/* Allocation of threads */
	threads = (pthread_t*) calloc(numberof_threads, sizeof(pthread_t));

	/* Thread creation */
	for (i = 0; i < numberof_threads; ++i){
        pthread_create(&threads[i], NULL, (void *) &produce_matrix, NULL);
    }	

    // /* Wait for thread to be killed */
    for(i = 0; i < numberof_threads; ++i){
    	pthread_join(threads[i], NULL);
    }    

    free(threads);

	return 0;
}

void produce_matrix(void* size){
	int error, error2;
	int BUFFER_NUMBER = BUF_NUM;
	int i,j;	
	float** m_alias; 
	
	while(!die_flag){
		//printf("flag5\n");
		//printf("p thread: %u waits for lock \n", (unsigned int) pthread_self());
		if(error = pthread_mutex_lock(&matrix_thread_lock) < 0){
			perror("matrix_thread_lock");
			exit(EXIT_FAILURE);
		}
		//printf("flag6\n");
		//printf("p thread: %u in critical section \n", (unsigned int) pthread_self());
		/******************* -Produce- *************************/
		printf("p thread: %u waits for lock \n", (unsigned int) pthread_self());
		if(error2 = pthread_mutex_lock(&prod_decom_lock) < 0){
			perror("matrix_thread_lock");
			exit(EXIT_FAILURE);
		}
		printf("p thread: %u in critical section \n", (unsigned int) pthread_self());
		//printf("flag7\n");
		printf("p before mbuf_index %d, BUFFER_NUMBER %d\n", mbuf_index, BUFFER_NUMBER);

		while((mbuf_index >= BUFFER_NUMBER) && !error){
			error2 = pthread_cond_wait(&produce, &prod_decom_lock);
		}
			printf("p out condition wait\n");
		if(error2){
			pthread_mutex_unlock(&prod_decom_lock);
			perror("prod_decom_lock");
			exit(EXIT_FAILURE);
		}
		
		++mbuf_index;
		matrix_buffer[mbuf_index] = calloc(1, sizeof(matrix_t));

		m_alias = matrix_buffer[mbuf_index]->m;
		m_alias = calloc(size_of_matrices, sizeof(float*));
		
		for(i = 0; i < size_of_matrices; ++i){
			m_alias[i] = calloc(size_of_matrices, sizeof(float));
		}

		srand(time(NULL));
		for(i = 0; i < size_of_matrices; ++i){
			for (j = 0; j < size_of_matrices; ++j){								
				m_alias[i][j] = abs((rand())%50);
			}
		}
		printf("p after mbuf_index: %d\n", mbuf_index);
	 //    for(i = 0; i < size_of_matrices; i++){
		// 	for(j = 0; j < size_of_matrices; j++)
		// 		printf("%6.2f ", m_alias[i][j]);
		// 	printf("\n");
		// }
		
		printf("flag1\n");
		if(error2 = pthread_cond_signal(&consume)){
			pthread_mutex_unlock(&prod_decom_lock);
			perror("prod_decom_unlock");
			exit(EXIT_FAILURE);
		}
		printf("flag2\n");
		if(error2){
			pthread_mutex_unlock(&prod_decom_lock);
			perror("prod_decom_unlock");
			exit(EXIT_FAILURE);
		}
		printf("p thread: %u out of prod_decom_lock \n", (unsigned int) pthread_self());
		/**************************************************************/

		//printf("thread: %u in critical section \n", (unsigned int) pthread_self());
		printf("flag3\n");
		if(error = pthread_mutex_unlock(&matrix_thread_lock) < 0){
			perror("matrix_thread_lock");
			exit(EXIT_FAILURE);
		}
		//printf("flag4\n");
	}
	printf("thread bitti\n");
}

void print_usage(){
	puts("Usage: ./module1 <size of matrices> <# of threads>");
	puts("Example: ./module1 20 5");
}
int l_u_decomposer(){
	int i;
	pthread_t* threads;
	float matrixL[size_of_matrices][size_of_matrices],
		  matrixU[size_of_matrices][size_of_matrices];

	/* Create threads for LU decomposing */	  
	threads = calloc(numberof_threads, sizeof(pthread_t));
	
	for(i = 0; i < numberof_threads; ++i){
		pthread_create(&threads[0], NULL, (void*) decompose, NULL);
	}

	for(i = 0; i < numberof_threads; ++i){
		pthread_join(threads[i], NULL);
	}		  

}
void decompose(void* arg){
	int error, error2;
	int BUFFER_NUMBER = BUF_NUM;
	int i;
	float** u_alias;
	float** l_alias;

	while(!die_flag){
		// printf("d thread: %u waits for lock \n", (unsigned int) pthread_self());

		if(error = pthread_mutex_lock(&decompose_thread_lock) < 0){
			perror("matrix_thread_lock");
			exit(EXIT_FAILURE);
		}
		// printf("d thread: %u in critical section \n", (unsigned int) pthread_self());
		/*************************************************************************/

		/******************* -Consume- *************************/
		printf("d thread: %u waits for lock \n", (unsigned int) pthread_self());
		if(error2 = pthread_mutex_lock(&prod_decom_lock) < 0){
			perror("matrix_thread_lock");
			exit(EXIT_FAILURE);
		}
		printf("d thread: %u in critical section \n", (unsigned int) pthread_self());

		printf("d before mbuf_index %d, BUFFER_NUMBER %d\n", mbuf_index, BUFFER_NUMBER);

		while((mbuf_index <= 0) && !error)
			error2 = pthread_cond_wait(&consume, &prod_decom_lock);
		printf("d out condition wait\n");
		if(error2){
			pthread_mutex_unlock(&prod_decom_lock);
			perror("prod_decom_lock");
			exit(EXIT_FAILURE);
		}
		
		/******************** Allocation for lu_matrices *************************/
		++lu_buf_index;
		lu_buffer[lu_buf_index] = (lu_t*)calloc(1, sizeof(lu_t));

		l_alias = lu_buffer[mbuf_index]->l;
		u_alias = lu_buffer[mbuf_index]->u;

		l_alias = calloc(size_of_matrices, sizeof(float*));
		u_alias = calloc(size_of_matrices, sizeof(float*));
		
		for(i = 0; i < size_of_matrices; ++i){
			l_alias[i] = calloc(size_of_matrices, sizeof(float));
			u_alias[i] = calloc(size_of_matrices, sizeof(float));
		}
		/*************************************************************************/

		/* Copy incoming matrix from matrix buffer to new LU matrices */
		memcpy(&l_alias[0],  &matrix_buffer[mbuf_index]->m[0], size_of_matrices*size_of_matrices*sizeof(float));
		memcpy(&u_alias[0],  &matrix_buffer[mbuf_index]->m[0], size_of_matrices*size_of_matrices*sizeof(float));

		/********************** deallocate unusued buffer matrix ****************************************/
		for(i = 0; i < size_of_matrices; ++i){
			free(matrix_buffer[mbuf_index]->m[i]);
		}
		free(matrix_buffer[mbuf_index]->m);
		free(matrix_buffer[mbuf_index]);
		--mbuf_index;
		/**********************************************************************************/
		printf("after decompose mbuf_index %d\n", mbuf_index);

		if(error2 = pthread_cond_signal(&produce)){
			pthread_mutex_unlock(&prod_decom_lock);
			perror("prod_decom_unlock");
			exit(EXIT_FAILURE);
		}
		if(error2){
			pthread_mutex_unlock(&prod_decom_lock);
			perror("prod_decom_unlock");
			exit(EXIT_FAILURE);
		}
		printf("d thread: %u out of prod_decom_lock \n", (unsigned int) pthread_self());
		/*************************************************************************/
		if(error = pthread_mutex_unlock(&decompose_thread_lock) < 0){
			perror("matrix_thread_lock");
			exit(EXIT_FAILURE);
		}
	}
}

int inversTaker(){

}