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
#include <time.h>
#include <sys/time.h>

#define BILLION 1000000000L
#define MILLION 1000000L
#define MY_FIFO "/tmp/"
#define LOGFILE "LOG"
#define MAX_PATH 512

/* flag for signal handling */
volatile sig_atomic_t die_flag = 0; 

/* This static function to handle specified signal */
static void static_func(int signo) {
        
    /* The signal flag */    
    die_flag = 1;
    printf("singal has caught! Please wait....\n");    
}

int main(int argc, char const *argv[])
{
	struct timespec server_timer;	
	FILE* logfile;
	int serverfd;
	int clientfd;
	char client_life_str[MAX_PATH];
	long sendingTime;
	int time_of_round = 0;
	char clientpid[6];
	char server_fifo_path[MAX_PATH]; // fifo dosyasının yolu
	char child_fifo_path[MAX_PATH];
	char chpid[6];		
	pid_t pid;
	char send;
	// /* Signal handling */
 //    if (signal(SIGINT, static_func) == SIG_ERR) {
 //        fputs("An error occurred while setting a signal handler.\n", stderr);
 //        return EXIT_FAILURE;
 //   	}
   	
   	printf("Welcome to my server\nMy pid: %d\n", (int)getpid());
   	/* Log file created */
   	// if( (logfile = fopen(LOGFILE, "w")) == NULL ){
   	// 	perror("fopen");
   	// 	exit(EXIT_FAILURE);
   	// }
   	
   	/* rate of sending data to clients */
   	sendingTime = atol(argv[1]);

   	server_timer.tv_sec = sendingTime/BILLION;
	server_timer.tv_nsec = sendingTime - (server_timer.tv_sec*BILLION );
	
	printf("server server_timer: %ld %ld\n", server_timer.tv_sec, server_timer.tv_nsec);

   	/**************** Fifo creating **************/
	sprintf(server_fifo_path, "%s%d", MY_FIFO, getpid());	
	printf("server_fifo_path: %s\n", server_fifo_path);
	
	/* Create and open fifo */
	if( mkfifo(server_fifo_path, 0666) < 0){
		perror("fifo not created:");
		exit(EXIT_FAILURE);	
	}

	if( (serverfd = open(server_fifo_path, O_RDWR)) < 0){
		perror("open fifo");
		exit(EXIT_FAILURE);
	}
	/*********************************************/

	while(1){
		
		/* Read client's message */
		if(read(serverfd, clientpid, 6) < 0){
			perror("read fifo");
			exit(EXIT_FAILURE);
		}
		printf("client pid: %s\n", clientpid);

		if(read(serverfd, client_life_str, MAX_PATH) < 0){
			perror("read fifo");
			exit(EXIT_FAILURE);
		}

		/* Gelen zaman milisaniye cinsinden olacak */
		printf("gelen zaman: %ld\n", atol(client_life_str));
		printf("sending time: %ld\n", sendingTime);
		time_of_round = (atol(client_life_str))/(sendingTime/1000);
		printf("time_of_round %d\n", time_of_round);
		//Zamanı saniye + nanosaniye cinsinden al		

		/* If message come, create fifo for that client */
		if( 0 > (pid = fork()) ){
			perror("fork");
			exit(EXIT_FAILURE);
		}

		/* Child will send result of math operations */
		if(pid == 0){
			sprintf(child_fifo_path, "%s%s", MY_FIFO, clientpid); 
			//printf("my child fifo path: %s \n", child_fifo_path);		

			/* Create fifo for each client */
			if( mkfifo(child_fifo_path, 0666) < 0){
				perror("fifo not created:");
				exit(EXIT_FAILURE);	
			}	
			//printf("child_fifo oluştu \n");
			/* Client fifo açıldı */
			if( (clientfd = open(child_fifo_path, O_WRONLY)) < 0){
				perror("open fifo");
				exit(EXIT_FAILURE);
			}
			//printf("clientfd açıldı. %d\n", clientfd);

			while(time_of_round--)
			{
				if( write(clientfd, "nelan", sizeof("nelan")) < 0)
					break;
				printf("yazdı\n");
				nanosleep(&server_timer, NULL);
			}
			printf("yazma işi bitti\n");
			close(clientfd);
			exit(EXIT_SUCCESS);
		}
		/* Parent don't waits for this client, but listens new clients */
		else{

			
		}

	}
	close(serverfd);
	unlink(server_fifo_path);
	return 0;
}


