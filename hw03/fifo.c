/** Name:	fifo.example.c
** Author:	K. Reek
** Contributor:	Warren Carithers
** Description:	Illustrate problems with bidirectional FIFOs
** SCCS ID:	@(#)fifo.example.c	1.2	01/30/02
** Compilation:	$com cc $@ -o $*
*/

#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <sys/errno.h>
#include <fcntl.h>

#define	MAX_LINE_LEN	100

int main( int ac, char **av ) {
	char buf[ MAX_LINE_LEN ];
	int	len;
	int	fifo;
	int	test = 1;

	if( ac > 1 ) {
		test = atoi( av[ 1 ] );
	}
	printf( "Running test %d\n", test );

	/*
	** Create the FIFO (ignore the error if it already exists).
	*/
	if( mkfifo( "F", 0666 ) < 0 ) {
		if( errno != EEXIST ) {
			perror( "mkfifo" );
			exit( EXIT_FAILURE );
		}
	}

	/*
	** Create the other process
	*/
	switch( fork() ) {
	case -1:
		perror( "fork" );
		exit( EXIT_FAILURE );

	case 0:	/* child process */
		do_child( test );
		/* NOTREACHED */

	default: /* parent process */
		break;
	}

	/*
	** Open the fifo
	*/
	if( test == 3 || test == 4 ) {
		sleep( 1 );
	}
	fifo = open( "F", O_RDWR );
	if( fifo < 0 ) {
		perror( "parent, open FIFO" );
		quit( EXIT_FAILURE );
	}
	printf( "parent has FIFO open\n" );

	/*
	** Write something to the FIFO, then read the other process' reply.
	*/
	printf( "parent writing to FIFO\n" );
	if( write( fifo, "Thank you", 10 ) < 0 ) {
		perror( "parent write to FIFO" );
		quit( EXIT_FAILURE );
	}
	printf( "parent write complete\n" );
	if( test <= 3 ) {
		sleep( 2 );
	}
	printf( "parent reading from FIFO\n" );
	len = read( fifo, buf, sizeof( buf ) );
	printf( "Parent got %d bytes: %s\n", len, buf );
	quit( EXIT_SUCCESS );
}
/* If anything goes wrong, we want to ensure that both processes die */
void quit( int result ) {
	killpg( getpgrp(), SIGINT );
	exit( result );
}

/* Child process: read something from the FIFO and send back a reply */
void do_child( int test ) {
	int	fifo;
	char	buf[ 100 ];
	int	len;

	if( test == 2 || test == 6 ) {
		sleep( 1 );
	}

	fifo = open( "F", O_RDWR );
	if( fifo < 0 ) {
		perror( "child FIFO open" );
		quit( EXIT_FAILURE );
	}
	printf( "child has FIFO open\n" );
	if( test == 1 || test == 5 ) {
		sleep( 1 );
	}

	/*
	** Read from the FIFO, then reply.
	*/
	printf( "child reading from FIFO\n" );
	len = read( fifo, buf, sizeof( buf ) );
	if( len < 0 ) {
		perror( "child read from FIFO" );
		quit( EXIT_FAILURE );
	}
	printf( "child got %d bytes: %s\nchild writing to FIFO\n", len, buf );
	if( write( fifo, "You're welcome", 15 ) < 0 ) {
		perror( "child write to FIFO" );
		quit( EXIT_FAILURE );
	}
	printf( "child write complete\n" );
	exit( 0 );
}



