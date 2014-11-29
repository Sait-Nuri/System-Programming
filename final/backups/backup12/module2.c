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

void multiplier(void*);
void matrixMultiplier(float[40][40], float[40][40], float[40][40], int);
void receiver(void* arg);
void rs_module1(void* arg);
void rs_verifier(void* arg);

struct sockaddr_in mult_server, module_addr, verify_addr;

pthread_mutex_t mult_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t module1_verify_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t threadrace_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t buffering_lock = PTHREAD_MUTEX_INITIALIZER;

int NUM_OF_THREAD;
int BUFFER_SIZE;
int MATRIX_SIZE;
int mult_sock;
int module1_sock;
int verify_sock;
int buffer_size = 0;
float first_buf[40][40]; 
float sec_buf[40][40]; 
float multiplied[40][40];

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
    pthread_t boss_thread;

	if(argc < 4 || argc > 4){
		puts("Invalid operation!");
		print_usage();
		exit(EXIT_FAILURE);
	}

	NUM_OF_THREAD = atoi(argv[1]);
	BUFFER_SIZE = atoi(argv[2]);
    MATRIX_SIZE = atoi(argv[3]);

	/******************* Socket Creation *****************************/
    
    if( (mult_sock = socket(AF_INET , SOCK_STREAM , 0)) < 0){
        perror("mult_sock");
        printf("Multiplier module exited...\n");
        exit(EXIT_FAILURE);
    }
    puts("mult_sock created");
     
    setsockopt(mult_sock,SOL_SOCKET,SO_REUSEADDR,NULL, 0);
     
    //Prepare the sockaddr_in structure
    memset(&mult_server, 0, sizeof(mult_server));

    // mult_server.sin_addr.s_addr = htonl("127.0.5.5");
    mult_server.sin_addr.s_addr = htonl(INADDR_ANY); //inet_addr(ipAdress);
    mult_server.sin_family = AF_INET;
    mult_server.sin_port = htons( 11112 );

    if( bind(mult_sock, (struct sockaddr *)&mult_server , sizeof(struct sockaddr)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        exit(EXIT_FAILURE);
    }

    //Listen
    listen(mult_sock , 5);

    puts("Waiting for verify module connections...");

    c = sizeof(struct sockaddr_in);

    /* Waiting for accepting Module1 */
    /* Waiting for accepting Verify module */
    verify_sock = accept(mult_sock, (struct sockaddr *)&verify_addr, (socklen_t*)&c);
    if (verify_sock < 0){
        perror("accept failed");
        exit(EXIT_FAILURE);
    }
    puts("Verify module accepted");

    module1_sock = accept(mult_sock, (struct sockaddr *)&module_addr, (socklen_t*)&c);
    if (module1_sock < 0){
        perror("accept failed");
        exit(EXIT_FAILURE);
    }
    puts("module1 accepted");
    

    /***************** End of socket creation *******************************/

    /******************* Thread creation part ******************************/

    /* Allocation of threads */
	threads = (pthread_t*) calloc(NUM_OF_THREAD, sizeof(pthread_t));

    pthread_create(&boss_thread, NULL, (void *) &receiver, NULL);

	/* Thread creation */
	for (i = 0; i < NUM_OF_THREAD; ++i){
        pthread_create(&threads[i], NULL, (void *) &multiplier, NULL);
    }	

    pthread_join(boss_thread, NULL);

    // /* Wait for thread to be killed */
    for(i = 0; i < NUM_OF_THREAD; ++i){
    	pthread_join(threads[i], NULL);
    }    
    free(threads);
    /********************** End of threads *******************************/

    close(verify_sock);
    close(mult_sock);
    close(module1_sock);

	return 0;
}
void receiver(void* arg){
    pthread_t formodule1;
    pthread_t forverify;

    pthread_create(&formodule1, NULL, (void *)&rs_module1, NULL);
    pthread_create(&forverify, NULL, (void *)&rs_verifier, NULL);

    pthread_join(formodule1, NULL);
    pthread_join(forverify, NULL);


}
void multiplier(void* arg){

    while(!die_flag){
        //Race of threads
        if(pthread_mutex_lock(&threadrace_lock) < 0){
            perror("threadrace_lock");
            exit(EXIT_FAILURE);
        }

        //Buffer is empty, try again
        if(buffer_size == 0){
           if(pthread_mutex_unlock(&threadrace_lock) < 0){
                perror("threadrace_lock");
                exit(EXIT_FAILURE);
            }
            //sleep(1);
            continue; 
        }

        if(pthread_mutex_lock(&buffering_lock) < 0){
            perror("buffering_lock");
            exit(EXIT_FAILURE);
        }
        //printf("d thread: %u in critical section \n", (unsigned int) pthread_self());

        --buffer_size;
        //printf("buffer_size %d\n", buffer_size);
        /*
            multiply and sending

        */
        if(pthread_mutex_unlock(&buffering_lock) < 0){
            perror("buffering_unlock");
            exit(EXIT_FAILURE);
        }

        //Race of threads
        if(pthread_mutex_unlock(&threadrace_lock) < 0){
            perror("threadrace_unlock");
            exit(EXIT_FAILURE);
        }   

    }  

}


void rs_module1(void* arg){
    int i, j;

    while(!die_flag){
        /*
        Reveice part
        */
        if(pthread_mutex_lock(&module1_verify_lock) < 0){
            perror("module1_verify_lock");
            exit(EXIT_FAILURE);
        }

        if(buffer_size == 1){
            if(pthread_mutex_unlock(&module1_verify_lock) < 0){
                perror("module1_verify_unlock");
                exit(EXIT_FAILURE);
            }
            //sleep(1);
            continue;
        }

        if(pthread_mutex_lock(&buffering_lock) < 0){
            perror("buffering_lock");
            exit(EXIT_FAILURE);
        }
        printf("rs_module1 thread: %u in critical section \n", (unsigned int) pthread_self());

            
        /********************* Receive part *******************************************/
        for(i = 0; i < MATRIX_SIZE; ++i){
            memset(first_buf[i], 0, sizeof(float)*40*40);
            memset(sec_buf[i], 0, sizeof(float)*40*40);
        }

        for(i = 0; i < MATRIX_SIZE; ++i){
            recv(module1_sock , first_buf[i] , sizeof(float)*MATRIX_SIZE , 0);
        }

        for(i = 0; i < MATRIX_SIZE; ++i){
            recv(module1_sock , sec_buf[i] , sizeof(float)*MATRIX_SIZE , 0);
        }

        // printf("received u\n");
        // for(i = 0; i < MATRIX_SIZE; ++i){
        //     for (j = 0; j < MATRIX_SIZE; ++j){
        //         printf("%6.6f ", first_buf[i][j]);
        //         }
        //     printf("\n");
        // }
        // sleep(1);

        // printf("received l\n");

        // for(i = 0; i < MATRIX_SIZE; ++i){
        //     for (j = 0; j < MATRIX_SIZE; ++j){
        //         printf("%6.6f ", sec_buf[i][j]);
        //         }
        //     printf("\n");
        // }
        // sleep(2);
        /*----------------------------------------------------------------*/

        matrixMultiplier(first_buf, sec_buf, multiplied, MATRIX_SIZE);
        /************************* Send back part *********************************/

        printf("multiplied \n");
        for(i = 0; i < MATRIX_SIZE; ++i){
            send(module1_sock, multiplied[i], MATRIX_SIZE*sizeof(float), 0);        
            for(j = 0; j < MATRIX_SIZE; ++j){
                printf("%6.6f ", multiplied[i][j]);
            }
            printf("\n");
        // sleep(1);
        }

        /*-----------------------------------------------------------------*/
        ++buffer_size;
        // printf("buffer_size %d\n", buffer_size);
        

        if(pthread_mutex_unlock(&buffering_lock) < 0){
            perror("buffering_unlock");
            exit(EXIT_FAILURE);
        }

        if(pthread_mutex_unlock(&module1_verify_lock) < 0){
            perror("module1_verify_unlock");
            exit(EXIT_FAILURE);
        }   
    }
    
}
void rs_verifier(void* arg){

    while(!die_flag){
        /*
        Reveice part
        */
        if(pthread_mutex_lock(&module1_verify_lock) < 0){
            perror("module1_verify_lock");
            exit(EXIT_FAILURE);
        }

        if(buffer_size == 1){
            if(pthread_mutex_unlock(&module1_verify_lock) < 0){
                perror("module1_verify_unlock");
                exit(EXIT_FAILURE);
            }
            //sleep(1);
            continue;
        }

        if(pthread_mutex_lock(&buffering_lock) < 0){
            perror("buffering_lock");
            exit(EXIT_FAILURE);
        }

        printf("rs_verifier thread: %u in critical section \n", (unsigned int) pthread_self());

        ++buffer_size;
        // printf("buffer_size %d\n", buffer_size);
        /*
            buffering here
        */

        if(pthread_mutex_unlock(&buffering_lock) < 0){
            perror("buffering_unlock");
            exit(EXIT_FAILURE);
        }

        if(pthread_mutex_unlock(&module1_verify_lock) < 0){
            perror("module1_verify_unlock");
            exit(EXIT_FAILURE);
        }   
    }
}

/* Multiplies two matrix and holds the result matrix */
void matrixMultiplier( float mat1[40][40], float mat2[40][40], float mat3[40][40], int dim ){

  int i, j, k;

  for(i=0;i<dim;i++){
    for(j=0;j<dim;j++){
        mat3[i][j]=0;
        for(k=0;k<dim;k++){
            mat3[i][j]+=mat1[i][k]*mat2[k][j];
        }
    }
  }

}
void print_usage(){
	puts("Usage: ./module1 <NUM_OF_THREAD> <BUFFER_SIZE> <MATRIX_SIZE>");
	puts("Example: ./module1 2 50 5");
}