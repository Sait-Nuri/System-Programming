#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <termios.h>
#include <errno.h>
#include <signal.h>

#define MY_FIFO "/tmp/"
#define LOGNAME "clientLOG"
#define MAX_PATH 512
#define PID_LENGNT 6
#define MAX_TIME 20

typedef struct clientData
{
	int clientType;
	char clientPid[PID_LENGNT];
	char client_life_str[MAX_TIME];
};

typedef struct sentData
{
	double result;
	long elapsed_time;
};


/* flag for signal handling */
volatile sig_atomic_t die_flag = 0; 

/* This static function to handle specified signal */
static void static_func(int signo) {
        
    /* The signal flag */    
    die_flag = 1;
    printf("singal has caught! Please wait....\n");    
}

int main(int argc, char *argv[])
{	
	int serverfd;
	char serverfifo[MAX_PATH];
	char clientfifo[MAX_PATH];
	int clientfd;
	int clientstatusfd;
	char sendmypid[PID_LENGNT];	
	char received[20];
	char* mypid = argv[1];
	char* client_life = argv[2];
	struct clientData client1;
	struct sentData sentData1;

	/* Signal handling */
    if (signal(SIGINT, static_func) == SIG_ERR) {
        fputs("An error occurred while setting a signal handler.\n", stderr);
        return EXIT_FAILURE;
   	}

	sprintf(serverfifo, "%s%s", MY_FIFO, mypid);	
	printf("serverfifo: %s\n", serverfifo);
	printf("my pid: %d\n", getpid());

	
	/* Server fifo file has been openend */
	if( (serverfd = open(serverfifo, O_RDWR)) < 0){
		perror("open fifo");
		exit(EXIT_FAILURE);
	}
	
	/*  Derivative client */
	client1.clientType = 1; 

	/* client pid to string */
	sprintf(sendmypid, "%d", getpid());
	printf("sendmypid: %s\n", sendmypid);

	/* Initialize struct */
	strcpy(client1.clientPid, sendmypid);
	strcpy(client1.client_life_str, client_life);

	/* Send the struct to server */
	if(write(serverfd, (void*)&client1, sizeof(struct clientData)) < 0){
		perror("write2 failed:");
		exit(EXIT_FAILURE);
	}

	/*************** Open special client fifo file *************/
	/* Open the special fifo */
	sprintf(clientfifo, "%s%s", MY_FIFO, sendmypid);
	printf("clientfifo %s\n", clientfifo);
	/* client fifo opened */
	while((clientfd = open(clientfifo, O_RDONLY)) <= 0){
		if(errno != ENOENT){
			perror("open client fifo");
		 	exit(EXIT_FAILURE);
		}
	}
	close(clientfd);
	clientfd = open(clientfifo, O_RDONLY);
	
	printf("clientfd: %d\n", clientfd);
	/**********************************************************/

	// /*************** Open status fifo file *********************/
	strcat(clientfifo, "status");
	printf("clientfifo %s\n", clientfifo);

	while((clientstatusfd = open(clientfifo, O_WRONLY)) <= 0){
		if(errno != ENOENT){
			perror("open client fifo");
		 	exit(EXIT_FAILURE);
		}
	}
	close(clientstatusfd);

	clientstatusfd = open(clientfifo, O_WRONLY);
	printf("status fd: %d\n", clientstatusfd);
	
	/**********************************************************/
	// if( (clientfd = open(clientfifo, O_RDONLY)) < 0){
	// 	perror("open fifo");
	// 	exit(EXIT_FAILURE);
	// }

	/* Received results from server */
	while(read(clientfd, (void*)&sentData1, sizeof(struct sentData)) > 0){					
		if(die_flag){
			write(clientstatusfd, "0", 2);
			break;
		}

		printf("%lf %ld\n", sentData1.result, sentData1.elapsed_time);			

		write(clientstatusfd, "1", 2);	
	}
	
	close(clientstatusfd);
	close(clientfd);
	return 0;
}