server : myserver.o
	cc -o server myserver.o -lrt -lm

client : myclient.o
	cc -o client myclient.o

myserver.o : myserver.c
	cc -c myserver.c

myclient.o : myclient.c	
	cc -c myclient.c

clean :
	rm  *.o 
