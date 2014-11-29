#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    int i;

    if (argc < 2)
    {
      printf("USAGE: %s loop-iterations\n", argv[0]);
      return 1;
    }

    int iterations = atoi(argv[1]);

    struct timeval start, end;

    gettimeofday(&start, NULL);

    for (i = 0; i < iterations*1000000; i++)
    {
    }

    gettimeofday(&end, NULL);

    printf("%ld\n", end.tv_sec - start.tv_sec);
    // printf("%ld\n", ((end.tv_sec * 1000000 + end.tv_usec)
    // 	  - (start.tv_sec * 1000000 + start.tv_usec)));

    return 0;
}