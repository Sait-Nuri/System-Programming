/*######################################################################*/
/*																		*/
/*	File Name:	tellMeMore.c											*/
/*																		*/
/*	Compile:	gcc -c tellMeMore.c										*/
/*	Link:		gcc tellMeMore.o -o tellMeMore							*/
/*	Run:		./tellMeMore chDirName									*/
/*																		*/
/*######################################################################*/

/*######################################################################*/
/*								Includes								*/
/*######################################################################*/
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <termios.h> 

/*######################################################################*/
/*								Defines									*/
/*######################################################################*/
#define MY_FIFO "/tmp/Server"
#define MESSAGE_FIFO "/tmp/messageFifo"
#define SERVER_LOG_FILE "/tmp/Server.log"
#define TRUE 1
#define FALSE 0
#define STRING_SIZE 4096

/*######################################################################*/
/*								Typedefs								*/
/*######################################################################*/
typedef char* chPtr;
typedef struct
{
	pid_t pid;
	pid_t chServerPid;
	char chKey[STRING_SIZE];
}search_t;

/*######################################################################*/
/*							Function Prototypes							*/
/*######################################################################*/
int getNumberOfLines(chPtr chFileName);
void myWait(pid_t pidChild);
ssize_t r_read(int iFd, chPtr chBuf, size_t iSize);

/*######################################################################*/
/*							Global Variables							*/
/*######################################################################*/
search_t searchResults[256];
int iNumSearchs = 0, iMoreIsActive = 0;
struct termios oldt, newt;
FILE *fptrLogFile;
char chLogFileName[50];

/* Returns zero on success, nonzero on error. */
int main(int argc, char** argv)
{
	int iServerFd, iClientFd, iBytesRead, 
		iFindCommandEntered = 0, iCtr, iMessageFifoFd;

	char chCommand[STRING_SIZE], 
		 chKey[STRING_SIZE], 
		 chMessageToServer[STRING_SIZE], 
		 chPidString[20], 
		 chBuffer[STRING_SIZE], 
		 chTimeFileName[STRING_SIZE], 
		 chElapsedTime[STRING_SIZE],
		 chServerPid[STRING_SIZE];

	FILE *fptrTimeFile, *fptrLsResult;
	pid_t pidChild;
	time_t tCurrentTime;
	struct sigaction sigintAction, sigusr1Action;

	tCurrentTime = time(NULL);

	/* Usage check */
	if(argc != 1)
	{
		printf("Usage: %s \n", argv[0]);
		printf(" ----------------\n");
		printf("|    Commands    |\n");
		printf("|----------------|\n");
		printf("| find [keyword] |\n");
		printf("| list [keyword] |\n");
		printf("| kill [keyword] |\n");
		printf("| quit           |\n");
		printf(" ----------------\n");
		return 1;
	}

	/* Check whether server is available */
	if(open(MY_FIFO, O_WRONLY) == -1)
	{
		printf("Server is off, open the server.\n");
		unlink(chLogFileName);
		return 2;
	}
	

	for( ; ; )
	{

		pidChild = fork();

		/* Child code */
		if(pidChild == 0)
		{
			tCurrentTime = time(NULL);

			iServerFd = open(MY_FIFO, O_WRONLY);

			sprintf(chMessageToServer,"%d",getpid());
			/* Write pid and directory path to server */
			write(iServerFd, chMessageToServer, STRING_SIZE);

			sprintf(chPidString, "%s%d", "/tmp/", getpid());


			mkfifo(chPidString, 0666);
			iClientFd = open(chPidString, O_RDONLY);

			r_read(iClientFd, chBuffer, STRING_SIZE);
			
			while((iBytesRead = r_read(iClientFd, chBuffer, STRING_SIZE)) != 0)
			{
				printf("reiss\n");
				close(iClientFd);
				close(iServerFd);
			}
		}
		/* Fork error check */
		else if(pidChild == -1)
		{
			printf("Failed to fork\n");
			exit(5);
		}

		/* Parent code */
		else
		{
			myWait(pidChild);
		}
	}
	
	return 0;

}

/* Waits for child */void myWait(pid_t pidChild)
{
	do
	{
		while( ( pidChild = wait(NULL) ) == -1 && ( errno == EINTR ) )
			;
	}while(pidChild > 0);
}


ssize_t r_read(int iFd, chPtr chBuf, size_t iSize) 
{
   ssize_t iRetVal;
   
   while ( (iRetVal = read(iFd, chBuf, iSize)) == -1 && errno == EINTR) ;
   
   return iRetVal;
}
/*######################################################################*/
/*							End of tellMeMore.c							*/
/*######################################################################*/