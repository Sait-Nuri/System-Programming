/*      Said Nuri UYANIK  	  */
/* 		  	101044067         */
/* 	   CSE 244 HW02 PART1     */
/* ls command with forking    */

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

void dirmanager(char *dirname, int shift);

int main(int argc, char *argv[])
{
	if(argc < 2)
		dirmanager("deneme", 2);
	else
		dirmanager(argv[1], 1);

return 0;
}

void dirmanager(char *dirname, int shift)
{
	struct dirent * dirInf; //This is a structure type used to return information about directory entries
	struct stat statInf;   //struct stat is a system struct that is defined to store information about files. 
	DIR *dp;
	pid_t cpid;
	char cwd[120];
	char *buf;
	char piper;
	int pipefd[2];
	int i;
	
	dp = opendir( dirname ); //open current directory
	if (dp == NULL) {
		return;
	}
    
	if (chdir( dirname ) < 0) {
		return;
	}
	
	cpid = fork(); //forking starts here
    if (cpid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
	
	if(cpid == 0){
		while( dirInf = readdir( dp ) ){

			if (! strcmp( dirInf->d_name, "." ) || ! strcmp( dirInf->d_name, ".." ))
				continue;
	
			if (stat( dirInf->d_name, &statInf ) < 0) //Finds information about a file.
				continue;

			switch (statInf.st_mode & S_IFMT) {
				case S_IFDIR: //if a directory. 				
						for(i = 2; i < shift; i++)
							printf("| \t");
						printf( "child: %s\n", dirInf->d_name);
						dirmanager( dirInf->d_name, shift+1);
						chdir(".."); 
						break;
			}//switch	
		}//while
	chdir(dirname);
	exit(EXIT_SUCCESS);	
	}//childpid

	else{
		wait(NULL);
		rewinddir(dp);
		while( dirInf = readdir( dp ) ){
			if (! strcmp( dirInf->d_name, "." ) || ! strcmp( dirInf->d_name, ".." ))
					continue;
	
			if (stat( dirInf->d_name, &statInf ) < 0) //Finds information about a file.
				continue;

			for(i = 2; i < shift; i++)
				printf("| \t");

			switch (statInf.st_mode & S_IFMT) {
				case S_IFREG: //if a regular file.
					buf = malloc(sizeof(char)*strlen(dirInf->d_name));
					strcpy(buf, dirInf->d_name);
					if( buf[strlen(buf)-1] != '~')	 
						printf( "parent: %s\n", dirInf->d_name);
					break;
			}//switch	
		}//while
	chdir(dirname);
	}//else
}//dirmanager()
