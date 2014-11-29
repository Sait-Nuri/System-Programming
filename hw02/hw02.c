#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>

/* USAGE */
void print_usage(char*);			 

/* Searchs the grepkey in indicated path "dirname" from indicated file location "logfile" */
int dirmanager(char*, char*, FILE*); 

/* Checks whether given string includes ".txt" or no */
int isText(char*);

/* Simply doing string matching with given strings 
   return value: 0 if str2 doesnt include in str1, 1 or more if includes */
int stringmatching(char*, char*);

/* String comparison starting index 0 to n */
int mystrncmp(char*, char*, int);

/* Prints the line inclues given keyword in given file location */
void grepMe(FILE*, char*, char*);

/* More operation with specific lenght in given file location */
void more(FILE*, int);

int main(int argc, char *argv[])
{	
	FILE * logfile;
	char* grepkey;			/* Word to grep in files */
	char* init_dirname;		/* initial directory in which grepping files */
	char* grepOption;   	/* -g */
	char* lengthOption; 	/* -l */
	int moreLineNum;        /* Number of lines to print the screen */
	char curDirName[1024];	

	/* Checking command line argumant number*/
	if(argc != 6){
		printf("\nMissing operand!\n");
		print_usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	/* Initializing the varibles */
	init_dirname = argv[1];
	grepOption = argv[2];
	grepkey = argv[3];
	lengthOption = argv[4];
	moreLineNum = atoi(argv[5]);

	/* Getting current directory name*/
	if( getcwd(curDirName, sizeof(curDirName)) == NULL ){
		perror("getcwd() error");
		exit(EXIT_FAILURE);	
	}
	
	/* Cheching command line argumants */
	if(grepOption[0] != '-' || lengthOption[0] != '-'){
		printf("\nInvalid character\n");
		print_usage(argv[0]);
		exit(EXIT_FAILURE);
	}	
	
	/* Open the file */					
	if( (logfile = fopen("/tmp/logfile", "a")) < 0 ){
		perror("logfile");
		exit(EXIT_FAILURE);
	}

	printf("Files are reading... Please wait...");
	printf("\r                                   \r");
	dirmanager(init_dirname, grepkey, logfile);
	fclose(logfile);

	more(logfile, moreLineNum);
	
	system("rm /tmp/logfile");

	return 0;
}
void more(FILE* logfile, int moreLineNum){

	char ch;	
	char buf;
	int i = 0;

	/* Creates the log file */
	if( (logfile = fopen("/tmp/logfile", "r")) < 0 ){
		perror("logfile");
		exit(EXIT_FAILURE);
	}

	while((ch=fgetc(logfile)) != EOF)
	{
		if(ch == '\n'){         
			i++;			
		}

		putchar(ch);

		if(i == moreLineNum)
		{
			printf("--MORE--");
			while(1)
			{			
				buf = (char)getch();
				if(buf == '\n'){
					i = 0;					
					break;
				}
				else if(buf == 'q' || buf == 'Q'){
					fclose(logfile);
					printf("\r          \r");
					printf("\n");
					return;
				}
			}
			printf("\r          \r");
		}
    }

	fclose(logfile);

}

/* Searchs the grepkey in indicated path "dirname" from indicated file location "logfile" */
int dirmanager(char* dirname, char* grepkey, FILE* logfile){
			
	pid_t new_pid;      			/* Pid number for fork */
	pid_t cpid;						/* Child pid */
	struct dirent * dirContent; 	/* This is a structure type used to return information about directory entries */
	struct stat dirInf;   			/* Struct stat is a system struct that is defined to store information about files. */ 
	DIR *dirP;						/* Directory structure*/	
	int errno;						/* Error no status */
	int child_status;				/* Keeps child status when child process exits */	
	char cur_file[512];

	/* Open current directory */
	if ( (dirP = opendir( dirname )) == NULL) {
		printf("\n%s cannot open\n", dirname);		
		perror("opendir()");
		exit(EXIT_FAILURE);			
	}

	/* Change the current directory to initial directory */
	if (chdir( dirname ) < 0) {
		perror("chdir() error");
		exit(EXIT_FAILURE);	
	}	

	/* Traverse the all directory entry */
	while(dirContent = readdir(dirP)){		

		/* If directory is "." or "..", skip this */
		if (! strcmp( dirContent->d_name, "." ) || ! strcmp( dirContent->d_name, ".." ))
			continue;				

		/* If there is no permission to read status, skip this */
		if ( stat( dirContent->d_name, &dirInf ) < 0) {
			continue;						
		}
		//printf("%s\n", dirContent->d_name);
		switch(dirInf.st_mode & S_IFMT){

			/* IS DIRECTORY*/
			case S_IFDIR:
				//printf("folder: %s\n", dirContent->d_name);
				/* If it is a directory, call dirmanager recursively*/							
				dirmanager(dirContent->d_name, grepkey, logfile);

				/* Come back to previous working directory */
				chdir("..");
				break;
			
			/* IS REGULAR FILE*/
			case S_IFREG: 	
				
				if(isText(dirContent->d_name) == 0 || dirContent->d_name[strlen(dirContent->d_name)-1] == '~'){					
					break;				
				}								
				/*************** -Forking part- *****************/
				/* Create new fork */				
				if( (new_pid = fork()) < 0 ){
					perror("fork"); 
					return 0;
				}

				/* Child process */
				if (new_pid == 0){														
					
					/* Grep works in here */					
					grepMe(logfile, grepkey, dirContent->d_name);										

					exit(EXIT_SUCCESS);
				}
				else{  /* Parent process */
				 	
				 	wait(&child_status);
				}
				/*************************************************/
				break;
				
		}/* switch */		
	} /* while */	
	closedir(dirP);

	return 1;
}

/* This function get a character but does not print it on the screen */
int getch( ) {

	struct termios before, after;
	int ch;

	tcgetattr( STDIN_FILENO, &before );
	after = before;
	after.c_lflag &= ~( ICANON | ECHO );
	tcsetattr( STDIN_FILENO, TCSANOW, &after );
	ch = getchar();
	tcsetattr( STDIN_FILENO, TCSANOW, &before );

	return ch;
}

/* Grep function */
void grepMe(FILE* logfile, char* grepkey, char* filename){

	char filepath[512];				/* Keeps a file path */
	char *buffer = NULL;			/* a line read from the file */
	size_t len = 0;					/* line length but not important */
	ssize_t read;					/* Keeps the lenght of line read from file. */
	pid_t cpid;						/* Child pid */
	int file_status = 0;	    	/* Keeps file status */
	int same_key_number;	    	/* Keeps the number of keywords in the same line */
	int line_num = 1;				/* Number of line */
	FILE* curfp;					/* File descripter of "filename" */
	int flag = 1;					

	/* Get child pid number */
	cpid = getpid();		

	/* Get path of the file */
	getcwd(filepath, sizeof(filepath));
	strcat(filepath, "/");
	strcat(filepath, filename);

	/* Open "filename" file */
	if( (curfp = fopen(filename, "r")) < 0){
		perror(filename);
		exit(EXIT_FAILURE);
	}	

	// fprintf(logfile, "\nFile name: %s", filename);	
	while( (read = getline(&buffer, &len, curfp)) != EOF ){						

		/* if the line is not empty, search the key in it */
		if(strlen(buffer)>0){
			file_status = 1;			
			
			/* Search the key in the line, returns number of how many times does the word include in the line */
			same_key_number = stringmatching(buffer, grepkey);			

			/* Print properties of the file */
			if(flag == 1 && same_key_number > 0){
				fprintf(logfile, "-----------------------------------------------------------\n" );
				fprintf(logfile, "File path: %s\nchild pid: %d\n\n", filepath, cpid);															
				flag = 2;
			}
										
			/* Write the lines to the file */
			while(same_key_number--){			
				fprintf(logfile, "#%d: %s", line_num, buffer); 					
			}								
		}		
		line_num++; 		
	}
	if(flag == 2){
		fprintf(logfile, "------------------------------------------------------------\n\n" );
		flag == 0;
	}
	
	free(buffer);
	fclose(curfp);		
}

int isText(char* filename){	
	return stringmatching(filename, ".txt");
}

/* 
	str1, str2 => Strings to match
	return value: 0 if str2 doesnt include in str1, 1 or more if includes
*/
int stringmatching(char* str1, char* str2){
	
	int str1_len = strlen(str1);
	int str2_len = strlen(str2);
	int amount_of_match = (str1_len - str2_len);
	int same_key_number = 0;
	int i;

	for (i = 0; i <= amount_of_match; ++i)
	{
		if( mystrncmp(&str1[i], str2, str2_len) ){
			same_key_number++;
		}
	}

	return same_key_number;
}

/*
	str1   => First string to be matched
	str2   => Secon string to match with str1
	lenght => Number of character to compare (0 to lenght)
*/
int mystrncmp(char* str1, char* str2, int lenght){
	
	int i;

	for (i = 0; i < lenght; ++i)
	{		
		if(str1[i] != str2[i])
			return 0;
	}

	return 1;
}

/* USAGE */
void print_usage(char* programName){
	printf("---------------------------------------------------------\n");
	printf("Usage: %s [FILE] -g [\"GREPKEY\"] -l [LENGTH]\n", programName);
	printf("FILE:    File name or file path\n");
	printf("GREPKEY: Keyword to find in files\n");
	printf("LENGTH:  Number of lines to print \n");
	printf("---------------------------------------------------------\n");
}