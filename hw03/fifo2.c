#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#define MAX_SIZE 256
#define FIFO_PATH "/tmp/myfifo"

volatile sig_atomic_t die_flag = 0;

static void catch_function(int signo) {
    
    puts("Interactive attention signal caught.");
    // if(signo == SIGINT){
    //     printf("gelen sinyal SIGINT\n");
    // }
    die_flag = 1;
    // if (raise(SIGKILL) != 0) {
    //     fputs("Error raising the signal.\n", stderr);
    //     exit(EXIT_FAILURE);
    // }

}

int main()
{
    int fd;
    pid_t pid;
    char * myfifo = "/tmp/myfifo";
    char buffer[MAX_SIZE];

    printf(FIFO_PATH);
    /* Signal handling */
    if (signal(SIGINT, catch_function) == SIG_ERR) {
        fputs("An error occurred while setting a signal handler.\n", stderr);
        return EXIT_FAILURE;
   }

    /* create the FIFO (named pipe) */
    mkfifo(myfifo, 0666);

    pid = fork();

    if( pid == -1){
        perror("fork");
    }

    /* Child process */
    if (pid == 0)
    {
        printf("child pid: %d\n", getpid());
        //printf("fd: %d\n", fd);

        /* Open fifo file to write only */
        if( (fd = open(myfifo, O_WRONLY)) < 0)
            perror("open fifo");            
        
        printf("fd: %d\n", fd);        
        memset(buffer, 0, MAX_SIZE); 

        //while(!die_flag){    
            printf("gir: ");
            scanf("%s", buffer);

            if(write(fd, buffer, strlen(buffer)+1) < 0)
                perror("read");

            //printf("write: %d\n", strlen(buffer));

            // flush the buffer
            memset(buffer, 0, MAX_SIZE); 
        //}
        close(fd);
        exit(1);
    }
    /* Parent process */
    else{
        printf("parent pid: %d\n", getpid());
        //printf("fd: %d\n", fd);
        
        /* Open fifo file to read only */
        if( (fd = open(myfifo, O_RDONLY)) < 0)
            perror("open fifo");             
        
        //while(!die_flag){    
            memset(buffer, 0, MAX_SIZE); 

            if(read(fd, buffer, MAX_SIZE) < 0)
                perror("read");

            //printf("read: %d\n", strlen(buffer));
            printf("Received: %s\n", buffer);

            // flush the buffer
            memset(buffer, 0, MAX_SIZE); 
        //}
        
        wait(NULL);
        close(fd);

        /* remove the FIFO */
        unlink(myfifo);
    }

    return 0;
}