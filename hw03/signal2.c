#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
 
static int flag;

static void catch_function(int signo) {
    puts("Interactive attention signal caught.");
    if(signo == SIGINT){
        printf("gelen sinyal SIGINT\n");
    }
    if (raise(SIGINT) != 0) {
        fputs("Error raising the signal.\n", stderr);
        exit(EXIT_FAILURE);
    }

}
 
int main(void) {
    if (signal(SIGINT, catch_function) == SIG_ERR) {
        fputs("An error occurred while setting a signal handler.\n", stderr);
        return EXIT_FAILURE;
    }
    while(1){

    }
    // puts("Raising the interactive attention signal.");
    // if (raise(SIGINT) != 0) {
    //     fputs("Error raising the signal.\n", stderr);
    //     return EXIT_FAILURE;
    // }
    puts("Exiting.");
    return 0;
}