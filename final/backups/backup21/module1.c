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
#include <sys/socket.h>    //socket
#include <arpa/inet.h> //inet_addr

#define BUF_NUM 100; 
#define DEFAULT_IP "127.0.0.1"

typedef struct matrix{
	float** m;
}matrix_t;

typedef struct lu{
	float** l;
	float** u;
	float** m;
}lu_t;

void print_usage();
void produce_matrix(void*);
int randomMatrixProducer(void*);
int l_u_decomposer();
void decompose(void* arg);
void decomposing(float**, float**, float**, int);
int inversTaker();
void inversing(void*);

float determinant(float**  ,int );
void cofactor(float** ,float );
void transpose(float** , float** , float );

void send_to_multiplier(float** , float** );
void get_multiplied(float** );
void send_to_verifier(float** , float** );

pthread_mutex_t decompose_thread_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t matrix_thread_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t prod_decom_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t inverser_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t decom_inv_lock = PTHREAD_MUTEX_INITIALIZER;

matrix_t *matrix_buffer;
lu_t * lu_buffer;
lu_t * invd_lu_buffer;

struct timespec sleeper;
struct sockaddr_in mult_addr;
struct sockaddr_in verify_addr;

int mult_sock;
int verify_sock;
int mbuf_index = -1;
int lu_buf_index = -1;
int NUM_OF_THREAD;
int MATRIX_SIZE;
int QUEUE_SIZE;
int MAX_PRODUCE;
int DEBUG;
int produce_counter = 0;
int decomposed_counter = 0;
int inversed_counter = 0;
int negative = -1;

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
	const char* MULTPLR_IP;
	const char* VERIFY_IP;

	pthread_t rmp_thread, decom_thread, invtak_thread;

	/* Command line argumant control */
	if(!(argc == 6 || argc == 8)){
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

	/* Maximum produce */
	if((MAX_PRODUCE = atoi(argv[2])) < 1){
		printf("Invalid number of TOTAL MATRIX TO BE PROCESSED!\n");
		exit(EXIT_FAILURE);	
	}
	printf("MAX_PRODUCE %d\n", MAX_PRODUCE);

	if((QUEUE_SIZE = atoi(argv[3])) < 1){
		printf("Invalid number of QUEUE_SIZE!\n");
		exit(EXIT_FAILURE);	
	}	
	printf("QUEUE_SIZE %d\n", QUEUE_SIZE);

	if((NUM_OF_THREAD = atoi(argv[4])) < 1){
		printf("Invalid number of NUM_OF_THREAD!\n");
		exit(EXIT_FAILURE);	
	}	
	printf("NUM_OF_THREAD %d\n", NUM_OF_THREAD);

	if( (DEBUG = atoi(argv[5])) > 1){
		printf("Invalid DEBUG OPTION\n");
		printf("Type 1 to enable DEBUG MOD, type 0 else\n");
		exit(EXIT_FAILURE);	
	}

	if(argc == 8){
		MULTPLR_IP = argv[6];
		VERIFY_IP = argv[7];
	}
	

	/************** Create socket and connect ***************************/
	
    if((mult_sock = socket(AF_INET , SOCK_STREAM , 0)) < 0){
    	 printf("Could not create mult_sock");
    }
    if((verify_sock = socket(AF_INET , SOCK_STREAM , 0)) < 0){
    	 printf("Could not create mult_sock");
    }
    puts("Sockets created");
	
	if(argc == 8){
		mult_addr.sin_addr.s_addr = inet_addr(MULTPLR_IP);
		verify_addr.sin_addr.s_addr = inet_addr(VERIFY_IP);
	}     
    mult_addr.sin_addr.s_addr = inet_addr(DEFAULT_IP);
    mult_addr.sin_family = AF_INET;
    mult_addr.sin_port = htons( 6666 );

    verify_addr.sin_addr.s_addr = inet_addr(DEFAULT_IP);
    verify_addr.sin_family = AF_INET;
    verify_addr.sin_port = htons( 5432 );

    if(DEBUG && (argc == 8)){
    	printf("MULTPLR_IP %s\n", MULTPLR_IP);
    	printf("VERIFY_IP %s\n", VERIFY_IP);	
    }

    //Connect to remote server
    while( (connect(verify_sock , (struct sockaddr *)&verify_addr , sizeof(verify_addr))) != 0 && (!die_flag)){
        printf("trying to connect verify server...\n");
    }

    if(!die_flag)
    	puts("Connected\n");

    while( (connect(mult_sock , (struct sockaddr *)&mult_addr , sizeof(mult_addr))) != 0 && (!die_flag) ){
    	printf("trying to connect multiplier server...\n");
    }
        
    if(!die_flag)
    	puts("Connected\n");

    /*****************************************************************************/
	printf("------------ DEBUGGING MOD ACTIVATED ----------\n");

	matrix_buffer = calloc(QUEUE_SIZE, sizeof(matrix_t));
	lu_buffer = calloc(QUEUE_SIZE, sizeof(lu_t));
	invd_lu_buffer = calloc(QUEUE_SIZE, sizeof(lu_t));

	/* Create threads for each module */
	pthread_create(&rmp_thread, NULL, (void*) &randomMatrixProducer, NULL);
	pthread_create(&decom_thread, NULL, (void*) &l_u_decomposer, NULL);
	pthread_create(&invtak_thread, NULL, (void*) &inversTaker, NULL);

	pthread_join(rmp_thread, NULL);
	pthread_join(decom_thread, NULL);
	pthread_join(invtak_thread, NULL);

	// for(i = 0; i <= mbuf_index; ++i){
	// 	for (j = 0; j < MATRIX_SIZE; ++j)	{
	// 		free(matrix_buffer[i].m[j]);
	// 	}
	// 	free(matrix_buffer[i].m);		
	// }

	// for(i = 0; i <= lu_buf_index; ++i){
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
	pthread_t *threads;	

	/* Allocation of threads */
	threads = (pthread_t*) calloc(NUM_OF_THREAD, sizeof(pthread_t));

	/* Thread creation */
	for (i = 0; i < NUM_OF_THREAD; ++i){
        pthread_create(&threads[i], NULL, (void *) &produce_matrix, NULL);
    }	

    // /* Wait for thread to be killed */
    for(i = 0; i < NUM_OF_THREAD; ++i){
    	pthread_join(threads[i], NULL);
    }    
    free(threads);
    printf("randomMatrixProducer\n");
	return 0;
}

void produce_matrix(void* size){
	int error, error2;
	int i,j;	
	srand(time(NULL));

	while(!die_flag){
		//printf("flag5\n");
		// printf("p thread: %u waits for lock \n", (unsigned int) pthread_self());
		if(pthread_mutex_lock(&matrix_thread_lock) < 0){
			perror("matrix_thread_lock");
			exit(EXIT_FAILURE);
		}
		
		/* If maximum produce number reached, exit */
		if(produce_counter >= MAX_PRODUCE){
			//printf("reached maximum produce number %d\n", produce_counter);
			pthread_mutex_unlock(&matrix_thread_lock);
			break;
		}

		//printf("flag6\n");
		// printf("p thread: %u in critical section \n", (unsigned int) pthread_self());
		/******************* -Produce- *************************/
		if(mbuf_index >= QUEUE_SIZE){
			if(pthread_mutex_unlock(&matrix_thread_lock) < 0){
				perror("matrix_thread_unlock");
				exit(EXIT_FAILURE);
			}
			// printf("mbuf_index higher than %d!. lu_buf_index %d\n", QUEUE_SIZE, lu_buf_index);
			continue;
		}
		
		//printf("p thread: %u waits for lock \n", (unsigned int) pthread_self());
		if(error2 = pthread_mutex_lock(&prod_decom_lock) < 0){
			perror("prod_decom_lock");
			exit(EXIT_FAILURE);
		}
		
		if(DEBUG)
			printf("thread: %u in critical section in producer module\n", (unsigned int) pthread_self());
		//printf("flag7\n");
		// printf("p before mbuf_index %d, QUEUE_SIZE %d\n", mbuf_index, QUEUE_SIZE);
		
		++mbuf_index;
		++produce_counter;

		if(DEBUG)
			printf("queue index in produser module %d\n", mbuf_index);

		// printf("produce_counter: %d\n", produce_counter);

		matrix_buffer[mbuf_index].m = calloc(MATRIX_SIZE, sizeof(float*));
		
		for(i = 0; i < MATRIX_SIZE; ++i){
			matrix_buffer[mbuf_index].m[i] = calloc(MATRIX_SIZE, sizeof(float));
		}
		

		/*********************** -PRODUCE PART- ********************************/
		
		for(i = 0; i < MATRIX_SIZE; ++i){
			for (j = 0; j < MATRIX_SIZE; ++j){								
				matrix_buffer[mbuf_index].m[i][j] = abs((rand())%50);
			}
		}
		// printf("p mbuf_index: %d\n", mbuf_index);
		// printf("p lu_buf_index %d\n", lu_buf_index);
		// printf("produce_counter %d\n", produce_counter);

		if(DEBUG){
			printf("prodoced matrix\n");
			for(i = 0; i < MATRIX_SIZE; i++){
				for(j = 0; j < MATRIX_SIZE; j++)
					printf("%6.2f ", matrix_buffer[mbuf_index].m[i][j]);
				printf("\n");
			}
			printf("-----------------------------\n");	
		}
	    
		//sleep(2);

		//printf("flag1\n");
		sleeper.tv_sec = 0;
		sleeper.tv_nsec = 1000000; //1 millisec

		if(error = pthread_mutex_unlock(&prod_decom_lock) < 0){
			perror("prod_decom_unlock");
			exit(EXIT_FAILURE);
		}
		nanosleep(&sleeper, NULL);
		//printf("flag2\n");
		// printf("p thread: %u out of prod_decom_lock \n", (unsigned int) pthread_self());
		/**************************************************************/
		if(DEBUG)
			printf("thread: %u in critical section \n", (unsigned int) pthread_self());
		
		//printf("flag2\n");
		if(error = pthread_mutex_unlock(&matrix_thread_lock) < 0){
			perror("matrix_thread_lock");
			exit(EXIT_FAILURE);
		}
		//printf("flag4\n");
	}
}

void print_usage(){
	puts("Usage: ./module1 <[N] MATRIX_SIZE> <[K] AMOUNT_OF_MATRIX_PRODUCE> <[L] QUEUE_SIZE> <[P] NUM_OF_THREAD> <T DEBUGGING MODE> <IP OF MULTILIER MODULE> <IP OF VERIFY MODULE>");
	puts("Example: ./module1 5 50 100 3 1");
}

int l_u_decomposer(){
	int i;
	pthread_t* threads;
	float matrixL[MATRIX_SIZE][MATRIX_SIZE],
		  matrixU[MATRIX_SIZE][MATRIX_SIZE];

	/* Create threads for LU decomposing */	  
	threads = calloc(NUM_OF_THREAD, sizeof(pthread_t));
	
	for(i = 0; i < NUM_OF_THREAD; ++i){
		pthread_create(&threads[i], NULL, (void*) decompose, NULL);
	}

	for(i = 0; i < NUM_OF_THREAD; ++i){
		pthread_join(threads[i], NULL);
	}		  

	free(threads);
}
void decompose(void* arg){
	int i, j;

	while(!die_flag){
		// printf("d thread: %u waits for lock \n", (unsigned int) pthread_self());
		// printf("d lu_buf_index before: %d\n", lu_buf_index);
		if(pthread_mutex_lock(&decompose_thread_lock) < 0){
			perror("decompose_thread_lock");
			exit(EXIT_FAILURE);
		}
		
		printf("thread: %u in critical section in decomposition \n", (unsigned int) pthread_self());

		/******************* -Consume- *************************/
		
		/* If there is no matrix to be decomposed in buffer, exit */
		// printf("decomposed_counter %d\n", decomposed_counter);

		if(decomposed_counter >= MAX_PRODUCE){
			pthread_mutex_unlock(&decompose_thread_lock);
			break;
		}

		if(mbuf_index < 0){
			if(pthread_mutex_unlock(&decompose_thread_lock) < 0){
				perror("decompose_thread_lock");
				exit(EXIT_FAILURE);
			}
			continue;
		}

		if(!(lu_buf_index <= QUEUE_SIZE)){
			// printf("thread: %u\n", (unsigned int)pthread_self());
			// printf("lu_buffer exceeded the size! %d\n", lu_buf_index);
			pthread_mutex_unlock(&decompose_thread_lock);
			continue;
		}

		// printf("d thread: %u waits for lock \n", (unsigned int) pthread_self());
		if(pthread_mutex_lock(&prod_decom_lock) < 0){
			perror("prod_decom_lock");
			exit(EXIT_FAILURE);
		}

		if(pthread_mutex_lock(&decom_inv_lock) < 0){
			perror("decom_inv_lock");
			exit(EXIT_FAILURE);
		}

		if(DEBUG)
			printf("thread: %u in critical section in decomposition module\n", (unsigned int) pthread_self());

		// printf("d before lu_buf_index %d, QUEUE_SIZE %d\n", lu_buf_index, QUEUE_SIZE);

		
		/******************** Allocation for lu_matrices *************************/
		// printf("p mbuf_index: %d\n", mbuf_index);
		// printf("p lu_buf_index %d\n", lu_buf_index);
		// printf("decomposed_counter %d\n", decomposed_counter);

		++lu_buf_index;
		// printf("bayrak1\n");
		if(DEBUG)
			printf("queue index in decomposition module %d\n", lu_buf_index);
		// sleep(1);

		// printf("bayrak2\n");
		lu_buffer[lu_buf_index].l = calloc(MATRIX_SIZE, sizeof(float*));
		lu_buffer[lu_buf_index].u = calloc(MATRIX_SIZE, sizeof(float*));
		lu_buffer[lu_buf_index].m = calloc(MATRIX_SIZE, sizeof(float*));

		// printf("bayrak3\n");
		for(i = 0; i < MATRIX_SIZE; ++i){
			lu_buffer[lu_buf_index].l[i] = calloc(MATRIX_SIZE, sizeof(float));
			lu_buffer[lu_buf_index].u[i] = calloc(MATRIX_SIZE, sizeof(float));
			lu_buffer[lu_buf_index].m[i] = calloc(MATRIX_SIZE, sizeof(float));
		}

		/*************************************************************************/
		// printf("bayrak4\n");
		//printf("d mbuf_index %d\n", mbuf_index);
		
		/* Copy incoming matrix from matrix buffer to new LU matrices */
		for(i = 0; i < MATRIX_SIZE; ++i){
			for(j = 0; j < MATRIX_SIZE; ++j){
				lu_buffer[lu_buf_index].l[i][j] = matrix_buffer[mbuf_index].m[i][j];
				lu_buffer[lu_buf_index].u[i][j] = matrix_buffer[mbuf_index].m[i][j];
				lu_buffer[lu_buf_index].m[i][j] = matrix_buffer[mbuf_index].m[i][j];
			}
		}

		// printf("original m\n");
		// for(i = 0; i < MATRIX_SIZE; ++i){
		// 	for(j = 0; j < MATRIX_SIZE; ++j){
		// 		printf("%6.5f ", lu_buffer[lu_buf_index].m[i][j]);
		// 	}
		// 	printf("\n");
		// }

		


		/* LU decomposition function */
		decomposing(lu_buffer[lu_buf_index].m, lu_buffer[lu_buf_index].l, lu_buffer[lu_buf_index].u, MATRIX_SIZE);

		// printf("bayrak5\n");
		/********************** deallocate unusued buffer matrix ****************************************/
		for(i = 0; i < MATRIX_SIZE; ++i){
			free(matrix_buffer[mbuf_index].m[i]);
		}
		// printf("bayrak6\n");
		free(matrix_buffer[mbuf_index].m);

		--mbuf_index;
		++decomposed_counter;
		/**********************************************************************************/
		if(DEBUG)
			printf("Queue index %d\n", mbuf_index);
		if(DEBUG)
			printf("decomposition counter %d\n", decomposed_counter);

		// printf("decomposed_counter %d\n", decomposed_counter);
		if(pthread_mutex_unlock(&decom_inv_lock) < 0){
			perror("decom_inv_unlock");
			exit(EXIT_FAILURE);
		}

		if(pthread_mutex_unlock(&prod_decom_lock) < 0){
			perror("prod_decom_unlock");
			exit(EXIT_FAILURE);
		}
		// printf("d thread: %u out of prod_decom_lock \n", (unsigned int) pthread_self());
		/*************************************************************************/
		if(pthread_mutex_unlock(&decompose_thread_lock) < 0){
			perror("decompose_thread_unlock");
			exit(EXIT_FAILURE);
		}
	}
}

void decomposing(float** a,float** l,float** u,int n){
	int i=0,j=0,k=0;
	for(i=0;i<n;i++)
	{
		for(j=0;j<n;j++)
		{
			if(j<i)
				l[j][i]=0;
			else
			{
				l[j][i]=a[j][i];
				for(k=0;k<i;k++)
				{
					l[j][i]=l[j][i]-l[j][k]*u[k][i];
				}
			}			
		}
		for(j=0;j<n;j++)
		{
			if(j<i)
				u[i][j]=0;
			else if(j==i)
				u[i][j]=1;
			else
			{
				u[i][j]=a[i][j]/l[i][i];
				for(k=0;k<i;k++)
				{
					u[i][j]=u[i][j]-((l[i][k]*u[k][j])/l[i][i]);	
				}
			}
		}
	}

	if(DEBUG){
		printf("---after decomposition---- \n");
		printf("Lower triangular matrix\n");
		for(i = 0; i < n; i++)
		{
			for(j = 0; j < n; j++)
				printf("%6.6f ", l[i][j]);
			printf("\n");
		}
		printf("\n");
		// sleep(2);
		printf("Upper triangular matrix\n");
		for(i = 0; i < n; i++)
		{
			for(j = 0; j < n; j++)
				printf("%6.6f ", u[i][j]);
			printf("\n");
		}
		printf("----------------------\n");
	}
		
	// sleep(2);	
}


int inversTaker(){

	int i;
	pthread_t *threads;	

	/* Allocation of threads */
	threads = (pthread_t*) calloc(NUM_OF_THREAD, sizeof(pthread_t));

	/* Thread creation */
	for (i = 0; i < NUM_OF_THREAD; ++i){
        pthread_create(&threads[i], NULL, (void *) &inversing, NULL);
    }	

    // /* Wait for thread to be killed */
    for(i = 0; i < NUM_OF_THREAD; ++i){
    	pthread_join(threads[i], NULL);
    }    

    free(threads);
    close(mult_sock);
    close(verify_sock);

	return 0;
}
void inversing(void* arg){
	int i, j;

	//sleep(1);

	while(!die_flag){

		if(pthread_mutex_lock(&inverser_lock) < 0){
			perror("inverser_lock");
			exit(EXIT_FAILURE);	
		}
		
		/* If there is no LU in buffer, exit */
		if(inversed_counter >= (MAX_PRODUCE)){
			// printf("Reached limit of number of inversed matrix: %d\n", inversed_counter);
			pthread_mutex_unlock(&inverser_lock);
			send(mult_sock, (void*)&negative, sizeof(int), 0);
			send(verify_sock, (void*)&negative, sizeof(int), 0);
			break;
		}

		if(lu_buf_index <= -1 && (inversed_counter < MAX_PRODUCE)){
			//printf("There is no matrix in lu_buffer anymore %d\n", lu_buf_index);
			pthread_mutex_unlock(&inverser_lock);
			continue;
		}
		
		// printf("i thread: %u waits for lock \n", (unsigned int) pthread_self());
		if(pthread_mutex_lock(&decom_inv_lock) < 0){
			perror("decom_inv_lock");
			exit(EXIT_FAILURE);
		}

		if(DEBUG)
			printf("thread: %u in critical section in inversing module\n", (unsigned int) pthread_self());

		//Burada sockete matrix gönderilecek
		/**********************************************/
		/*
			invers the matrix
		*/
		if(DEBUG){
			printf("\noriginal l: \n");
			for(i = 0; i < MATRIX_SIZE; ++i){
				for (j = 0; j < MATRIX_SIZE; ++j){
					printf("%6.6f ", lu_buffer[lu_buf_index].l[i][j]);
				}
				printf("\n");
			}
		cofactor(lu_buffer[lu_buf_index].l, MATRIX_SIZE);

			printf("\noriginal u: \n");
			for(i = 0; i < MATRIX_SIZE; ++i){
				for (j = 0; j < MATRIX_SIZE; ++j){
					printf("%6.6f ", lu_buffer[lu_buf_index].u[i][j]);
				}
				printf("\n");
			}
		}

		cofactor(lu_buffer[lu_buf_index].u, MATRIX_SIZE);	

		/*
			send original matrix and inversed matrix
		*/	

		send_to_multiplier(lu_buffer[lu_buf_index].u, lu_buffer[lu_buf_index].l);

		// /* Receive from multiplier */
		get_multiplied(lu_buffer[lu_buf_index].u);

		/* Send A and inversed A */
		send_to_verifier(lu_buffer[lu_buf_index].u, lu_buffer[lu_buf_index].m);

		/**********************************************/
		for(i = 0; i < MATRIX_SIZE; ++i){
			free(lu_buffer[lu_buf_index].l[i]);
			free(lu_buffer[lu_buf_index].u[i]);
			free(lu_buffer[lu_buf_index].m[i]);
		}
		// printf("bayrak6\n");
		free(lu_buffer[lu_buf_index].l);
		free(lu_buffer[lu_buf_index].u);
		free(lu_buffer[lu_buf_index].m);

		--lu_buf_index;
		++inversed_counter;

		if(DEBUG)
			printf("Queue index %d\n", lu_buf_index);
		if(DEBUG)
			printf("inversed matrix counter %d\n", inversed_counter);

		if(pthread_mutex_unlock(&decom_inv_lock) < 0){
			perror("decom_inv_unlock");
			exit(EXIT_FAILURE);
		}		

		/* Thread release lock, other threads will try to lock */
		if(pthread_mutex_unlock(&inverser_lock) < 0){
			perror("inverser_unlock");
			exit(EXIT_FAILURE);
		}
	}
}

/*
	a => original matrix, not changed

*/
void send_to_multiplier(float** u, float** l){
	int i,j;

	// printf("send size\n");
	send(mult_sock, (void*)&MATRIX_SIZE, sizeof(int), 0);

	//printf("sent u\n");
	for(i = 0; i < MATRIX_SIZE; ++i){
		send(mult_sock, u[i], MATRIX_SIZE*sizeof(float), 0);		
		// for(j = 0; j < MATRIX_SIZE; ++j){
		// 	printf("%6.5f ", u[i][j]);
		// }
		// printf("\n");
		//sleep(2);
	}
	//printf("\n");
	// printf("sent l\n");
	for(i = 0; i < MATRIX_SIZE; ++i){
		send(mult_sock, l[i], MATRIX_SIZE*sizeof(float), 0);
		// for(j = 0; j < MATRIX_SIZE; ++j){
		// 	printf("%6.5f ", l[i][j]);
		// }
		// printf("\n");
		//sleep(2);
	}
	// printf("\n");
	// sleep(1);

}
void get_multiplied(float** mult){
	int i, j;

	if(DEBUG)
		printf("Multiplication of inversed U and inversed L: \n");
	
	for(i = 0; i < MATRIX_SIZE; ++i){
        recv(mult_sock, mult[i], MATRIX_SIZE*sizeof(float), 0);
        if(DEBUG)
	        for(j = 0; j < MATRIX_SIZE; ++j){
	            printf("%6.5f ", mult[i][j]);
	        }
        	printf("\n");
    }
    if(DEBUG)
    	printf("---------------------------------\n");
    //sleep(2);
}

void send_to_verifier(float** invA, float** A){
	int i, j;

	// printf("lu_buf_index %d\n", lu_buf_index);
	// printf("inversed_counter %d\n", inversed_counter);

	send(verify_sock, (void*)&MATRIX_SIZE, sizeof(int), 0);

	// printf("sent A\n");
	for(i = 0; i < MATRIX_SIZE; ++i){
		send(verify_sock, A[i], MATRIX_SIZE*sizeof(float), 0);
		// for(j = 0; j < MATRIX_SIZE; ++j){
		// 	printf("%6.2f ", A[i][j]);
		// }
		// printf("\n");
		// sleep(1);
	}

	// printf("sent inversed A\n");
	for(i = 0; i < MATRIX_SIZE; ++i){
		send(verify_sock, invA[i], MATRIX_SIZE*sizeof(float), 0);		
		// for(j = 0; j < MATRIX_SIZE; ++j){
		// 	printf("%6.2f ", invA[i][j]);
		// }
		// printf("\n");
		// sleep(1);
	}
	// sleep(1);
}

float determinant(float** matrix ,int size)
{
    float s=1,det=0;
    float** b;
    int i,j,m,n,c;

    b = calloc(MATRIX_SIZE, sizeof(float*));

    for(i = 0; i < MATRIX_SIZE; ++i){
        b[i] = calloc(MATRIX_SIZE, sizeof(float));
    }

    
    if (size==1)
    {
        return (matrix[0][0]);
    }
    else
    {
        det=0;
        for (c=0;c<size;c++)
        {
            m=0;
            n=0;
            for (i=0;i<size;i++)
            {
                for (j=0;j<size;j++)
                {
                    b[i][j]=0;
                    if (i != 0 && j != c)
                    {
                        b[m][n]=matrix[i][j];
                        if (n<(size-2))
                            n++;
                        else
                        {
                            n=0;
                            m++;
                        }
                    }
                }
            }
            det=det + s * (matrix[0][c] * determinant(b,size-1));
            s=-1 * s;
        }
    }
    
    for(i = 0; i < MATRIX_SIZE; ++i){
        free(b[i]);
    }
    free(b);

    return (det);
}
 
void cofactor(float** matrix,float size)
{
    float** b; 
    float** fac; 
    int p,q,m,n,i,j;

    b = calloc(MATRIX_SIZE, sizeof(float*));
    fac = calloc(MATRIX_SIZE, sizeof(float*));

    for(i = 0; i < MATRIX_SIZE; ++i){
        b[i] = calloc(MATRIX_SIZE, sizeof(float));
        fac[i] = calloc(MATRIX_SIZE, sizeof(float));

    }      

    for (q=0;q<size;q++)
    {
	    for (p=0;p<size;p++)
	    {
	        m=0;
	        n=0;
	        for (i=0;i<size;i++)
	        {
	            for (j=0;j<size;j++)
	            {
	                if (i != q && j != p)
	                {
	                    b[m][n]=matrix[i][j];
	                    if (n<(size-2))
	                        n++;
	                    else
	                    {
	                       n=0;
	                       m++;
	                    }
	                }
	            }
	        }
	        fac[q][p]=pow(-1,q + p) * determinant(b,size-1);
	    }
  	}
  	transpose(matrix, fac, size);

    for(i = 0; i < MATRIX_SIZE; ++i){
        free(b[i]);
        free(fac[i]);

    }
    free(b);
    free(fac);
}
/*Finding transpose of matrix*/ 
void transpose(float** matrix, float** fac, float size)
{
    int i,j;
    float d;
    float temp;

    d = determinant(matrix,size);
    for (i=0;i<size;i++)
    {
        for (j=0;j<size;j++)
        {
            matrix[j][i]=(fac[i][j] / d);
        }
    }

    if(DEBUG){
    	printf("\nThe inverse of matrix is : \n");
	    for (i=0;i<size;i++)
	    {
	        for (j=0;j<size;j++)
	        {
	            printf("%6.6f ",matrix[i][j]);
	        }
	        printf("\n");
	    }
	    printf("---------------------------\n");
    }
    
}