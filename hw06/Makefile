all:
	gcc -c mserver.c -g
	gcc -c mclient.c -g	
	gcc -c mlocalserver.c -g
	gcc -o mserver mserver.o -lpthread -lrt 
	gcc -o mclient mclient.o -lpthread -lrt	-lm
	gcc -o mlocalserver mlocalserver.o -lpthread -lrt -lm
	rm *.o
clean :
	rm  *.o 
