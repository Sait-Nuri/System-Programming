#########################################################################
# BIL 244 System Programming                                           	#
# easy to follow Makefile example                                 	    #  
# for debuging compile with -g switch (type make CFLAGS=-g)             #
#########################################################################

run : fork_exec.o
	cc -o run fork_exec.o

fork_exec.o : fork_exec.c
	cc -c fork_exec.c

# Clean up build products.
clean :
	rm -f *.o run

# Makefile Tutorial : http://www.gnu.org/software/make/manual/make.html#Rule-Introduction
