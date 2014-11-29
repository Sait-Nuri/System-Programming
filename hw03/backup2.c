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
#include <fcntl.h>
#include <signal.h>

#define FIFO_PATH "/tmp/myfifo"  /* Fifo file path */
#define MAX_SIZE 512

/* flag for signal handling */
volatile sig_atomic_t die_flag = 0; 

/* This static function to handle specified signal */
static void static_func(int signo) {
        
    /* The signal flag */    
    die_flag = 1;
    printf("singal has caught! Please wait....\n");    
}

/* USAGE */
void print_usage(char*);			 

/* Searchs the grepkey in indicated path "dirname" from indicated file location "logfile" */
int dirmanager(char*, char*, int); 

/* Checks whether given string includes ".txt" or no */
int isText(char*);

/* Simply doing string matching with given strings 
   return value: 0 if str2 doesnt include in str1, 1 or more if includes */
int stringmatching(char*, char*);

/* String comparison starting index 0 to n */
int mystrncmp(char*, char*, int);

/* Prints the line inclues given keyword in given file location */
int grepMe(char*, char*, int);

/* More operation with specific lenght in given file location */
void more(int, int);

int main(int argc, char *argv[])
{		
	char* grepkey;				/* Word to grep in files */
	char* init_dirname;			/* initial directory in which grepping files */
	char* grepOption;   		/* -g */
	char* lengthOption; 		/* -l */
	char* myfifo = FIFO_PATH;   /* fifo file path */
	int fd;					    /* fifo file descriptor */	    
	int moreLineNum;        	/* Number of lines to print the screen */	
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

	/* Signal handling */
    if (signal(SIGINT, static_func) == SIG_ERR) {
        fputs("An error occurred while setting a signal handler.\n", stderr);
        return EXIT_FAILURE;
   	}

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

	printf("Files are reading... Please wait...");
	printf("\r                                   \r");
	
	if( fd = mkfifo(myfifo, 0666) < 0){
		perror("fifo not created:");
		exit(EXIT_FAILURE);	
	}

	/* Traversing the whole directory to grep */
	dirmanager(init_dirname, grepkey, moreLineNum);	
	
	/* Remove fifo file */
	unlink(myfifo);	

	return 0;
}

/* Searchs the grepkey in indicated path "dirname" from indicated file location "logfile" */
int dirmanager(char* dirname, char* grepkey, int moreLineNum){
			
	pid_t new_pid;      			/* Pid number for fork */
	pid_t cpid;						/* Child pid */
	struct dirent * dirContent; 	/* This is a structure type used to return information about directory entries */
	struct stat dirInf;   			/* Struct stat is a system struct that is defined to store information about files. */ 
	DIR *dirP;						/* Directory structure*/	
	int errno;						/* Error no status */
	int child_status;				/* Keeps child status when child process exits */	
	int fd;							/* File descriptor for fifo */
	int die_status;					/* Keeps the signal status */
	int pp[2];						/* Pipe */	
	size_t nbyte = 1;				/* Size of a character for  */
	char inform;					/* Information about child process to send pipe */

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

	/* Create pipe for communicate between parent and child about signal catching */
	if (pipe(pp) == -1) {
        perror("pipe");
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

		switch(dirInf.st_mode & S_IFMT){

			/* IS DIRECTORY*/
			case S_IFDIR:
				printf("folder: %s\n", dirContent->d_name);
				
				/* If it is a directory, call dirmanager recursively*/							
				if(die_flag){

					/* Close the directory before process killed */
					closedir(dirP);
					
					/* If kill signal is caught, kill own process */
					if (raise(SIGKILL) != 0) {
        				fputs("Error raising the signal.\n", stderr);
        				exit(EXIT_FAILURE);
    				}
				}

				/* Go into through the directory found (dirContent->d_name) */
				dirmanager(dirContent->d_name, grepkey, moreLineNum);
				
				/* Come back to previous working directory */
				chdir("..");
				break;
			
			/* IS REGULAR FILE*/
			case S_IFREG: 	
				printf("file %s\n", dirContent->d_name);
				
				/* Eliminate the files ending with '~'' and
				   Check the files whether are txt file or not */
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
				
					/* Child process closes up input side of pipe */
		            close(pp[0]);	

					/* Open fifo file to write only */
			        if( (fd = open(FIFO_PATH, O_WRONLY)) < 0)
		        		perror("open fifo");					
		        	
					/* Grep works in here */					
					if(!die_flag)
						die_status = grepMe(grepkey, dirContent->d_name, fd);										

					/********************* SIGNAL HANDLING PART ******************************/
					if(die_status){
						printf("\nI'm child, My pid: %d, Now I'm dying in peace\n", getpid());
						inform = '1'; //signal caught message					

	            		// /* Inform the parent process about dying of child process */	
	            		write(pp[1], (void*)&inform, nbyte);	            		
						
						/* Go and kill yourself */
						if (raise(SIGKILL) != 0) {
            				fputs("Error raising the signal.\n", stderr);        
        				}
					/*************************************************************************/
					}
				
					/* signal not caught message */
					inform = '0'; 

					/* Inform the parent process about dying of child process */	
            		write(pp[1], (void*)&inform, nbyte);		
            		
					/* Close the file descripter for child process */
					close(fd);

					/* Exit from child process */
					exit(EXIT_SUCCESS);
				}
				else{  /* Parent process */
					
					/* Open fifo file to write only */
					if( (fd = open(FIFO_PATH, O_RDONLY)) < 0)
            			perror("open fifo");  										

            		/* Read the pipe about signal status of child */
					read( pp[0], (void*)&inform, nbyte );					 					 									 					 	

					/* If signal has been caught by child, give the message to screen */
				 	if(inform == '1'){

				 		printf("I'm parent(%d), My child is dying\n", getpid());
				 	}
			
				 	/* Wait for child process to be killed */
				 	wait(&child_status);
				 	
				 	/* More operations */
				 	more(fd, moreLineNum);

				 	/* Close file descripter */
				 	close(fd);
				}
				/*************************************************/
				break;
				
		}/* switch */
		if(inform == '1')		
			break;
	} /* while */	
	closedir(dirP);

	return 1;
}

/* Grep function */
int grepMe(char* grepkey, char* filename, int fd){

	char filepath[MAX_SIZE];		/* Keeps a file path */
	char *file_buffer = NULL;		/* a line read from the file */
	char buffer_to_write[MAX_SIZE];	/* Buffer to write to a fifo file */
	size_t len = 0;					/* line length but not important */
	ssize_t read;					/* Keeps the lenght of line read from file. */
	pid_t cpid;						/* Child pid */
	int file_status = 0;	    	/* Keeps file status */	
	int same_key_number;	    	/* Keeps the number of keywords in the same line */
	int line_num = 1;				/* Number of line */
	FILE* curfp;					/* File descripter of "filename" */
	int flag = 1;					/* Flag is used to determine whether a file grepped or not */
	

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
	while( (read = getline(&file_buffer, &len, curfp)) != EOF ){								

		/* When kill signal is caught, return 1 to indicate catching kill signal*/
		if(die_flag){			
			break;
		}

		/* if the line is not empty, search the key in it */
		if(strlen(file_buffer)>0){
			file_status = 1;			

			/* Search the key in the line, returns number of how many times does the word include in the line */
			same_key_number = stringmatching(file_buffer, grepkey);												

			/* Print properties of the file */
			if(flag == 1 && same_key_number > 0){								
				
				/* Write informations to buffer */
				read = sprintf(buffer_to_write, 
				"-----------------------------------------------------------\nFile path: %s\nchild pid: %d\n\n", filepath, cpid);				
				
				/* Write buffer to fifo file fd */
				if(write(fd, buffer_to_write, read+1) < 0)
                	perror("write2 failed:");																	

				/* flag to add this => ---------- to end of part if grepkey is found at least one time in file_name */
				flag = 2;
			}
						
			/* now copy the grep results to buffer_to_write */
			read = sprintf(buffer_to_write, "#%d: %s", line_num, file_buffer);

			/* Write the lines to the file */
			while(same_key_number--){										
			
				/* Write buffer to fifo file fd */
				if(write(fd, buffer_to_write, read+1) < 0)
                	perror("write3 failed:");								
			}	
			
		}		
		line_num++; 		
	}
	if(flag == 2){

		/* Write separator to buffer_to_write */
		read = sprintf(buffer_to_write, "------------------------------------------------------------\n\n" );

		/* Write buffer_to_write to fifo file fd */
		if(write(fd, buffer_to_write, read+1) < 0)
            perror("write3 failed:");				

		flag == 0;
	}

	free(file_buffer);
	fclose(curfp);		

	/* if signal caught then return 1 or othwewise 0 */
	if(die_flag){			
		return 1;
	}else
		return 0;	
}

void more(int fd, int moreLineNum){

	char ch;					/* Keeps the user input*/
	char fifo_buffer[MAX_SIZE]; /* Buffer for fifo file*/
	int i=0;					/* A counter for characters read from fifo file */	
	int status = -1;			/* Status of read a fifo file */

	if(read(fd, fifo_buffer, 1) == -1){
		perror("read");
		exit(EXIT_FAILURE);
	}

	putchar(fifo_buffer[0]);

	while((status = read(fd, fifo_buffer, 1)) > 0)
	{		
		if(fifo_buffer[0] == '\n')
			i++;			
		
		putchar(fifo_buffer[0]);

		if(i == moreLineNum)
		{
			printf("--MORE--");
			while(1)
			{			
				ch = (char)getch();
				if(ch == '\n'){
					i = 0;					
					break;
				}
				else if(ch == 'q' || ch == 'Q'){					
					printf("\r          \r");
					printf("\n");
					return;
				}
			}
			printf("\r          \r");
		}
    }

    if(status == -1){
		perror("read() in more()");
		exit(EXIT_FAILURE);
	}	

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

/*
	filename: To compare with given string 
*/
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