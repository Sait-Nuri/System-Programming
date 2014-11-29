/* Example of using sigaction() to setup a signal handler with 3 arguments
 * including siginfo_t.
 */
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

volatile sig_atomic_t die_flag = 0;

static void hdl (int sig, siginfo_t *siginfo, void *context)
{
    printf ("Dying process: %d\n", getpid());    
    die_flag = 1;
    // /* Handling part */
    // if (raise(SIGKILL) != 0) {
    //     fputs("Error raising the signal.\n", stderr);        
    // }    
}
 
int main (int argc, char *argv[])
{
    struct sigaction act;
    pid_t pid;
    int status = 0;

    memset (&act, '\0', sizeof(act));
    printf("pid: %d\n", getpid());
    /* Use the sa_sigaction field because the handles has two additional parameters */
    act.sa_sigaction = &hdl;
 
    /* The SA_SIGINFO flag tells sigaction() to use the sa_sigaction field, not sa_handler. */
    act.sa_flags = SA_SIGINFO;
 
    if (sigaction(SIGTERM, &act, NULL) < 0) {
        perror ("sigaction");
        return 1;
    }
    
    pid = fork();
    if( pid == -1){
        perror("fork");
    }

    /* Child process */
    if (pid == 0)
    {
        printf("child pid: %d\n", getpid());    
        while(!die_flag){

        }
        printf("child dies :(\n");
        if (raise(SIGUSR1) != 0) {
            fputs("Error raising the signal.\n", stderr);        
        }
        return 2;    
    }
    else
    {
        printf("parent pid: %d\n", getpid());        
        wait(&status);        
        printf("parent decide to die\n");
        printf("child status: %d\n", status);
        if (raise(SIGKILL) != 0) {
            fputs("Error raising the signal.\n", stderr);        
        }   
    }
    
 
    return 0;
}

// #include <signal.h>
// #include <stdio.h>
// #include <stdlib.h>
 
// static int flag;

// static void catch_function(int signo) {
//     puts("Interactive attention signal caught.");
//     if(signo == SIGINT){
//         printf("gelen sinyal SIGINT\n");
//     }
//     // if (raise(SIGKILL) != 0) {
//     //     fputs("Error raising the signal.\n", stderr);
//     //     exit(EXIT_FAILURE);
//     // }

// }
 
// int main(void) {
//     if (signal(SIGINT, catch_function) == SIG_ERR) {
//         fputs("An error occurred while setting a signal handler.\n", stderr);
//         return EXIT_FAILURE;
//     }
//     while(1){

//     }
//     // puts("Raising the interactive attention signal.");
//     // if (raise(SIGINT) != 0) {
//     //     fputs("Error raising the signal.\n", stderr);
//     //     return EXIT_FAILURE;
//     // }
//     puts("Exiting.");
//     return 0;
// }