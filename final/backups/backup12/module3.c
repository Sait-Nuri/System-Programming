/* Verify module */

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

typedef struct lu{
	float** l;
	float** u;
}lu_t;


void print_usage();
void verifier(void*);
void get_matrices();

struct sockaddr_in verify_server, module1_client, mult_addr;

pthread_mutex_t threadrace_lock = PTHREAD_MUTEX_INITIALIZER;

int NUM_OF_THREAD;
int BUFFER_SIZE;
int MATRIX_SIZE;
int module1_sock;
int verify_sock;
int verify_sock2;

float matrixA[40][40];
float inversedA[40][40];

/* flag for signal handling */
volatile sig_atomic_t die_flag = 0;

/* This static function to handle specified signal */
static void signal_func(int signo) {
    /* The signal flag */    
    die_flag = 1;
    printf("singal has caught! Please wait....\n");    
}

int main(int argc, char const *argv[])
{
	int c, i;
	pthread_t* threads;

	if(argc < 4 || argc > 4){
		puts("Invalid operation!");
		print_usage();
		exit(EXIT_FAILURE);
	}

	NUM_OF_THREAD = atoi(argv[1]);
	BUFFER_SIZE = atoi(argv[2]);
    MATRIX_SIZE = atoi(argv[3]);

	/******************* Socket Creation *****************************/

    /************ Connection the multiplier server ************************/
    mult_addr.sin_family = AF_INET;
    mult_addr.sin_addr.s_addr = htonl(INADDR_ANY); //inet_addr(ipAdress);
    mult_addr.sin_port = htons( 11112 );

    if((verify_sock2 = socket(AF_INET , SOCK_STREAM , 0)) < 0){
        perror("verify_sock2");
        printf("MUltiplier module exited...\n");
        exit(EXIT_FAILURE);
    }
    puts("Socket created");

    while( (connect(verify_sock2 , (struct sockaddr *)&mult_addr , sizeof(mult_addr)) != 0) && (!die_flag)){
        printf("trying to connect multipler server\n");
    }

    puts("Connected to multiplier module\n");
    /***************************************************************/

    /*******************Accepting of module1*******************/
	if((verify_sock = socket(AF_INET , SOCK_STREAM , 0)) < 0){
        perror("verify_sock");
        printf("verify module exited...\n");
        exit(EXIT_FAILURE);
    }
    puts("Socket created");
       
     
    //Prepare the sockaddr_in structure
    verify_server.sin_family = AF_INET;
    verify_server.sin_addr.s_addr = htonl(INADDR_ANY); //inet_addr(ipAdress);
    verify_server.sin_port = htons( 5432 );
    
    //setsockopt(verify_sock, SOL_SOCKET, SO_REUSEADDR, NULL, sizeof(verify_sock));

    if( bind(verify_sock, (struct sockaddr *)&verify_server , sizeof(verify_server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        exit(EXIT_FAILURE);
    }

    //Listen
    listen(verify_sock , 3);

    puts("Waiting for module1 connect");

    c = sizeof(struct sockaddr_in);

    /* Waiting for accepting Verify module */
    module1_sock = accept(verify_sock, (struct sockaddr *)&module1_client, (socklen_t*)&c);
    if (module1_sock < 0){
        perror("accept failed");
        exit(EXIT_FAILURE);
    }
    puts("Module1 accepted");
    /************************* End of Accepiton **************************/

    /* Allocation of threads */
	threads = (pthread_t*) calloc(NUM_OF_THREAD, sizeof(pthread_t));

	/* Thread creation */
	for (i = 0; i < NUM_OF_THREAD; ++i){
        pthread_create(&threads[i], NULL, (void *) &verifier, NULL);
    }	

    // /* Wait for thread to be killed */
    for(i = 0; i < NUM_OF_THREAD; ++i){
    	pthread_join(threads[i], NULL);
    }    
    free(threads);
    
    close(module1_sock);
    close(verify_sock);
    close(verify_sock2);

	return 0;
}

void verifier(void* arg){
    int i;

    while(!die_flag){
        
        //Race of threads
        if(pthread_mutex_lock(&threadrace_lock) < 0){
            perror("threadrace_lock");
            exit(EXIT_FAILURE);
        }
        
        // for(i = 0; i < MATRIX_SIZE; ++i){
        //     memset(matrixA[i], 0, sizeof(float)*40*40);
        //     memset(inversedA[i], 0, sizeof(float)*40*40);
        // }

        // printf("d thread: %u in critical section \n", (unsigned int) pthread_self());
        /*
            receive matrix  A and A^ from 
        */
            get_matrices();
        /*
            send A and A^ to multiplier
        */    

        /*
            receive multiplied matrix and check whether unit matrix or not
        */    

        //Race of threads
        if(pthread_mutex_unlock(&threadrace_lock) < 0){
            perror("threadrace_unlock");
            exit(EXIT_FAILURE);
        }   

        // for(i = 0; i < MATRIX_SIZE; ++i){
        //     memset(matrixA[i], 0, sizeof(float)*40*40);
        //     memset(inversedA[i], 0, sizeof(float)*40*40);
        // }
    }  

}
void get_matrices(){
    int i, j;

    printf("get A\n");
    for(i = 0; i < MATRIX_SIZE; ++i){
        recv(module1_sock, matrixA[i], MATRIX_SIZE*sizeof(float), 0);
        for(j = 0; j < MATRIX_SIZE; ++j){
            printf("%6.2f ", matrixA[i][j]);
        }
        printf("\n");
        //sleep(2);
    }
        
    printf("get inversed A\n");
    for(i = 0; i < MATRIX_SIZE; ++i){
        recv(module1_sock, inversedA[i], MATRIX_SIZE*sizeof(float), 0);       
        for(j = 0; j < MATRIX_SIZE; ++j){
            printf("%6.2f ", inversedA[i][j]);
        }
        printf("\n");
        //sleep(2);
    }
}

void print_usage(){
    puts("Usage: ./module1 <NUM_OF_THREAD> <BUFFER_SIZE> <MATRIX_SIZE>");
    puts("Example: ./module1 5 50 3");
}