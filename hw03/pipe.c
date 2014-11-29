#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>


int main(void)
{
        int     fd[2], nbytes;
        pid_t   childpid;
        char    string[] = "Hello, world!\n";
        char    readbuffer[80];        
        char    ch;
        size_t nbyte = 1;

        pipe(fd);
        
        if((childpid = fork()) == -1)
        {
                perror("fork");
                exit(1);
        }

        if(childpid == 0)
        {
                /* Child process closes up input side of pipe */
                close(fd[0]);

                scanf("%c", &ch);                
                /* Send "string" through the output side of pipe */
                while(1){                    
                    write(fd[1], (void*)&ch, nbyte);                
                    if(ch == '\n')
                        break;                                        
                    scanf("%c", &ch);                                        
                }
                
                exit(0);
        }
        else
        {
                /* Parent process closes up output side of pipe */
                close(fd[1]);

                /* Read in a string from the pipe */
                while(1){                    
                    nbytes = read( fd[0], (void*)&ch, nbyte );
                    printf("%c", ch);
                    if(ch == '\n')
                        break;
                }
                
                wait(NULL);
        }
        
        return(0);
}