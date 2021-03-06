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
#define MAX_PATH 512
#define PID_LENGNT 6

int main(int argc, char *argv[])
{	
	int serverfd;
	char serverfifo[MAX_PATH];
	char clientfifo[MAX_PATH];
	int clientfd;
	char sendmypid[PID_LENGNT];
	int fifo_not_opened = 1;	
	char received[20];
	int counter = 5;
	char* mypid = argv[1];
	char* client_life = argv[2];

	sprintf(serverfifo, "%s%s", MY_FIFO, mypid);	
	printf("serverfifo: %s\n", serverfifo);
	printf("my pid: %d\n", getpid());

	/* Server fifo file has been openend */
	if( (serverfd = open(serverfifo, O_RDWR)) < 0){
		perror("open fifo");
		exit(EXIT_FAILURE);
	}
	
	/* Send client pid to server */
	sprintf(sendmypid, "%d", getpid());
	printf("sendmypid: %s\n", sendmypid);

	if(write(serverfd, sendmypid, PID_LENGNT) < 0){
		perror("write2 failed:");
		exit(EXIT_FAILURE);
	}

	/* Send lifetime of client */
	if(write(serverfd, client_life, strlen(client_life)) < 0){
		perror("write2 failed:");
		exit(EXIT_FAILURE);
	}

	/* Open the special fifo */
	sprintf(clientfifo, "%s%s", MY_FIFO, sendmypid);
	printf("clientfifo %s\n", clientfifo);
	/* client fifo opened */
	while(clientfd = open(clientfifo, O_RDONLY) == -1){
		
	}
	close(clientfd);

	clientfd = open(clientfifo, O_RDONLY);
	// if( (clientfd = open(clientfifo, O_RDONLY)) < 0){
	// 	perror("open fifo");
	// 	exit(EXIT_FAILURE);
	// }
	printf("flaaaag\n");
	/* Received results from server */
	while(read(clientfd, received, sizeof("ne lan")) > 0){		
		// if(read(clientfd, received, sizeof("ne lan")) < 0){			
		// 	break;
		// }	
		printf("okundu: %s\n", received);
	}
	
	close(clientfd);
	return 0;
}