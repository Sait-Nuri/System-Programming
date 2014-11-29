all:

	gcc -c derivClient.c
	gcc -c IntegClient.c
	gcc -c myserver.c
	gcc -o server myserver.o -lrt -lm
	gcc -o derivClient derivClient.o
	gcc -o IntegClient IntegClient.o 
	rm *.o
clean :
	rm  *.o 
