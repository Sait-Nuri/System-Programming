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
#define THOUSAND 1000L
#define MY_FIFO "/tmp/"
#define LOGNAME "serverLog"
#define MAX_PATH 512
#define MAX_TIME 20
#define PID_LENGNT 6

double getDeriv(struct timespec);

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

int main(int argc, char const *argv[])
{
	struct timespec server_timer;
	struct timespec start, end;	
	FILE* logfile;
	int serverfd;
	int clientfd;
	int statusfd;
	long server_send_rate;
	long elapsed;
	long delay;
	double derivRes;
	int time_of_round = 0;
	char client_life[MAX_TIME];
	char client_pid[PID_LENGNT];
	char server_fifo_path[MAX_PATH]; // fifo dosyasının yolu
	char child_fifo_path[MAX_PATH];
	char chpid[6];	
	char valid;
	pid_t pid;
	struct clientData client1;
	struct sentData sentData1;

	/* Signal handling */
    if (signal(SIGINT, static_func) == SIG_ERR) {
        fputs("An error occurred while setting a signal handler.\n", stderr);
        return EXIT_FAILURE;
   	}
   	
   	printf("Welcome to my server\nMy pid: %d\n", (int)getpid());
   	

   	/* Log file created */
   	if( (logfile = fopen(LOGNAME, "a")) == NULL ){
   		perror("fopen");
   		exit(EXIT_FAILURE);
   	}
   	
   	/********************* Server timer **********************************/
   	/* rate of sending data to clients */
   	server_send_rate = atol(argv[1]);

   	server_timer.tv_sec = server_send_rate/BILLION;
	server_timer.tv_nsec = server_send_rate - (server_timer.tv_sec*BILLION );
	printf("server server_timer: %ld %ld\n", server_timer.tv_sec, server_timer.tv_nsec);
	/**********************************************************************/
	

   	/**************** Fifo creating and openning **************/
	sprintf(server_fifo_path, "%s%d", MY_FIFO, getpid());	
	printf("server_fifo_path: %s\n", server_fifo_path);
	
	/* Create and open fifo */
	if( mkfifo(server_fifo_path, 0666) < 0){
		perror("fifo not created:");
		exit(EXIT_FAILURE);	
	}

	if( (serverfd = open(server_fifo_path, O_RDONLY)) < 0){
		perror("open fifo");
		exit(EXIT_FAILURE);
	}

	/***********************************************************/

	while(1){
		
		/***************Connect to Client*****************/

		/* Read client's message */
		while(!die_flag){
			if(read(serverfd, (void*)&client1, sizeof(struct clientData) ) > 0)
				break;
			else if(errno == ENOENT){
				perror("read fifo");
				exit(EXIT_FAILURE);
			}
		}
		// if(read(serverfd, (void*)&client1, sizeof(struct clientData) ) < 0){
		// 	perror("read fifo");
		// 	exit(EXIT_FAILURE);
		// }

		if(die_flag){
			break;
		}
		/*************************************************/

		clock_gettime(CLOCK_REALTIME,&start);		

		strcpy(client_life ,client1.client_life_str);
		strcpy(client_pid, client1.clientPid);

		/* Gelen zaman milisaniye cinsinden olacak */
		printf("gelen zaman: %ld\n", atol(client_life));
		/* gelen pid */
		//printf("client pid: %s\n", client_pid);

		/* Serverin veri gönderme sıklığı */
		//printf("server rate: %ld\n", server_send_rate);

		/**************** Server Starting time *********************/
		/* Toplamda kaç kere veri gönderilecek */
		time_of_round = (atol(client_life))/(server_send_rate/1000);
		//printf("time_of_round %d\n", time_of_round);
		
		//Zamanı saniye + nanosaniye cinsinden al		
		/***********************************************************/

		/* If message come, create fifo for that client */
		if( 0 > (pid = fork()) ){
			perror("fork");
			exit(EXIT_FAILURE);
		}

		/* Child will send result of math operations */
		if(pid == 0){			
			sprintf(child_fifo_path, "%s%s", MY_FIFO, client_pid); 
			//printf("my child fifo path: %s \n", child_fifo_path);		

			/***************** Create fifo for each client ****************/
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
			/*************************************************************/

			/**************Create status fifo for client *****************/
			strcat(child_fifo_path, "status");	
			//printf("status path %s\n", child_fifo_path);

			if( mkfifo(child_fifo_path, 0666) < 0){
				perror("fifo not created:");
				exit(EXIT_FAILURE);	
			}

			if( (statusfd = open(child_fifo_path, O_RDONLY)) < 0){
				perror("open fifo");
				exit(EXIT_FAILURE);
			}
			/*************************************************************/
			//printf("status fifo oluştu\n");

			while(time_of_round--)
			{
				if(die_flag)
					break;
				clock_gettime(CLOCK_REALTIME,&end);								

				/* Stub function of derivative always returns 1 */
				sentData1.result = getDeriv(end);		

				/* Send current micro second */
				sentData1.elapsed_time =  ((end.tv_sec - start.tv_sec)*MILLION) + (end.tv_nsec - start.tv_nsec)/1000;

				write(clientfd, (void*)&sentData1, sizeof(struct sentData));

				/* Check whether client gets an iterrupt or not*/
				printf("yazdı\n");
				read(statusfd, &valid, 2);
				//printf("valid: %c\n", valid);

				if(valid == '0'){
					printf("client %s is died\n", client_pid);
					break;
				}				
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

	fclose(logfile);
	close(statusfd);
	close(serverfd);
	unlink(server_fifo_path);
	return 0;
}
double getDeriv(struct timespec curTime){

	return 1.0;
}

