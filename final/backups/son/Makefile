all:
	gcc -c module1.c
	gcc -c module2.c
	gcc -c module3.c
	gcc -o module1 module1.o -lpthread -lm -lrt
	gcc -o module2 module2.o -lpthread -lm -lrt	
	gcc -o module3 module3.o -lpthread -lm -lrt	
	rm *.o
clean :
	rm  *.o 
