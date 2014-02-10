
/*
* Example of how to use fork and exec functions.
*/
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int 
main(int argc, char const *argv[])
{
	pid_t child_pid;

	printf("Parent            process pid = %d\n", (int)getpid() );

	child_pid = fork();
	/* Parent process */
	if (child_pid != 0){
		printf("It s here, parent process pid = %d\n", (int)getpid());
		printf("Child             process pid = %d\n",(int)child_pid);
	}else{
		printf("It s here,  child process pid = %d\n", (int)getpid() );
		printf("Child's     paret process pid = %d\n", (int)getppid() );
	}
	
	return 0;
}
