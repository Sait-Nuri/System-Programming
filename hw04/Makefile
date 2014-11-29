all:

	gcc -c derivClient.c
	gcc -c IntegClient.c
	gcc -c myServer.c
	gcc -o server myServer.o -lrt -lm -lpthread
	gcc -o derivClient derivClient.o
	gcc -o integClient IntegClient.o 
	rm *.o
clean :
	rm  *.o 
