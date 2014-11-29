/*######################################################################*/
/*																		*/
/*	File Name:	lsServer.c												*/
/*																		*/
/*	Compile:	gcc -c lsServer.c										*/
/*	Link:		gcc lsServer.o -o lsServer								*/
/*	Run:		./lsServer												*/
/*																		*/
/*																		*/
/*	This program serves files and directory names in a directory to 	*/
/*	clients.															*/
/*																		*/
/*######################################################################*/

/*######################################################################*/
/*								Includes								*/
/*######################################################################*/
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>

/*######################################################################*/
/*								Defines									*/
/*######################################################################*/
#define MY_FIFO "/tmp/Server"
#define SERVER_LOG_FILE "/tmp/Server.log"
#define CLIENT_FILE "/tmp/clients.txt"
#define TRUE 1
#define FALSE 0
#define FAILED -1
#define BILLION 1000000000L
#define THOUSAND 1000L
#define STRING_SIZE 4096

/*######################################################################*/
/*								Typedefs								*/
/*######################################################################*/
typedef char* chPtr;
typedef char** chPPtr;
typedef struct dirent* direntPtr;
typedef DIR* dirPtr;
typedef FILE* fPtr;

/*######################################################################*/
/*							Function Prototypes							*/
/*######################################################################*/
unsigned long getDifTime(struct timeval tStart, struct timeval tFinish);
void parsePidDirName(chPtr chMessage, chPtr chParentPid);
void sigintActionHandler(int iSigNum);
void sigusr1ActionHandler(int iSigNum);
void setUp();
void findFilesAndWriteToFifo(chPtr chPath, int iFdFifo);
int isDirectory(chPtr chPath);
void myWait(pid_t pidChild);
ssize_t r_write(int iFd, chPtr chBuf, size_t iSize);
ssize_t r_read(int iFd, chPtr chBuf, size_t iSize);
void sigchildActionHandler(int iSigNum);

/*######################################################################*/
/*							Global Variables							*/
/*######################################################################*/
int iFdServer, iFdCommonFifo;
fPtr fptrLogFile, fptrClientFile;
time_t currentTime;

/* Returns zero on success, nonzero on error. */
int main(int argc, char** argv)
{
	char chMessageFromClient[STRING_SIZE],
		 chClientFileName[15],
		 chElapsedTime[15],
		 chBuffer[STRING_SIZE],
		 chChildPid[10],
		 chParentPid[10];

	struct timeval  tStart, tFinish;
	pid_t pidChild;
	int iFdClient;

	/* Usage check */
	if(argc != 1)
	{
		printf("Usage: %s\n", argv[0]);
		return 1;
	}

	setUp();

	for( ; ; )
	{
		iFdServer = open(MY_FIFO, O_RDONLY);
		r_read(iFdServer, chMessageFromClient, STRING_SIZE);
		
		pidChild = fork();

		/* Child code */
		if(pidChild == 0)
		{
			parsePidDirName(chMessageFromClient, chParentPid);

			/* Empty string */
			if(strlen(chParentPid) == 0)
				break;

			printf("Client \"%s\" connected" ,chParentPid);
			sprintf(chClientFileName, "%s%s", "/tmp/", chParentPid);

			currentTime = time(NULL);
			
			iFdClient = open(chClientFileName, O_WRONLY);
			
			sprintf(chBuffer, "%d", getpid());
			write(iFdClient, chBuffer, STRING_SIZE);

			close(iFdClient);
			break;
		}

		/* Error check for fork */
		else if(pidChild == -1)
		{
			printf("Failed to fork\n");
			exit(5);
		}
		
		/* Parent code */
		else
		{
			myWait(pidChild);
			memset(chMessageFromClient, 0, sizeof(char) * STRING_SIZE);
		}
	}
	
	return 0;
}

/* Returns different time in nanoseconds. */
unsigned long getDifTime(struct timeval tStart, struct timeval tFinish)
{
	return ( BILLION * (tFinish.tv_sec - tStart.tv_sec) + 
			 THOUSAND * (tFinish.tv_usec - tStart.tv_usec) );
}

/* Parses the message to child pid, parent pid and directory name. */
void parsePidDirName(chPtr chMessage, chPtr chParentPid)
{
	int iCtr1 = 0, iCtr12 = 0;

	while(chMessage[iCtr1] != '\n')
	{
		chParentPid[iCtr1] = chMessage[iCtr1];
		++iCtr1;
	}

	chParentPid[iCtr1] = '\0';
	++iCtr1;
}

/* Sets up server, creates files and sets signals */
void setUp()
{
	time_t currentTime;

	mkfifo(MY_FIFO, 0666);
	currentTime = time(NULL);
}



/* Waits for process with giveb pid. */
void myWait(pid_t pidChild)
{
	do
	{
		while( ( pidChild = wait(NULL) ) == -1 && ( errno == EINTR ) )
			;
	}while(pidChild > 0);
}

ssize_t r_write(int iFd, chPtr chBuf, size_t iSize) 
{
   chPtr bufp;
   size_t bytesToWrite;
   ssize_t bytesWritten;
   size_t totalBytes;

   for (bufp = chBuf, bytesToWrite = iSize, totalBytes = 0;
        bytesToWrite > 0;
        bufp += bytesWritten, bytesToWrite -= bytesWritten) 
   {
      bytesWritten = write(iFd, bufp, bytesToWrite);
	  
      if ((bytesWritten == -1) && (errno != EINTR))
         return -1;
	  
      if (bytesWritten == -1)
         bytesWritten = 0;
	  
      totalBytes += bytesWritten;
   }
   return totalBytes;
}

ssize_t r_read(int iFd, chPtr chBuf, size_t iSize) 
{
   ssize_t iRetVal;

   while ( (iRetVal = read(iFd, chBuf, iSize)) == -1 && errno == EINTR) ;

   return iRetVal;
}
/*######################################################################*/
/*							End of lsServer.c							*/
/*######################################################################*/