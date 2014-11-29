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

struct sockaddr_in mult_server, module_addr, verify_addr;

pthread_mutex_t mult_lock = PTHREAD_MUTEX_INITIALIZER;

int NUM_OF_THREAD;
int BUFFER_SIZE;
int mult_sock;
int module1_sock;
int verify_sock;

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

	if(argc < 3 || argc > 3){
		puts("Invalid operation!");
		print_usage();
		exit(EXIT_FAILURE);
	}

	NUM_OF_THREAD = atoi(argv[1]);
	BUFFER_SIZE = atoi(argv[2]);

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
    mult_server.sin_port = htons( 55555 );

    if( bind(mult_sock, (struct sockaddr *)&mult_server , sizeof(struct sockaddr)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        exit(EXIT_FAILURE);
    }

    //Listen
    listen(mult_sock , 5);

    puts("Waiting for incoming connections...");

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
    

    /***************************************************************/

    /* Allocation of threads */
	threads = (pthread_t*) calloc(NUM_OF_THREAD, sizeof(pthread_t));

	/* Thread creation */
	for (i = 0; i < NUM_OF_THREAD; ++i){
        pthread_create(&threads[i], NULL, (void *) &multiplier, NULL);
    }	

    // /* Wait for thread to be killed */
    for(i = 0; i < NUM_OF_THREAD; ++i){
    	pthread_join(threads[i], NULL);
    }    
    free(threads);

    close(verify_sock);
    close(mult_sock);
    close(module1_sock);

	return 0;
}
void multiplier(void* arg){

}

void print_usage(){
	puts("Usage: ./module1 <NUM_OF_THREAD> <BUFFER_SIZE>");
	puts("Example: ./module1 5 50");
}