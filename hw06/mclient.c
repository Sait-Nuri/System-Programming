#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>    /* POSIX Threads */
#include <string.h>     /* String handling */
#include <semaphore.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <math.h>

#define MAX_LENGHT 512
#define SHMKEY 1234
#define SHMMSGKEY 4321
#define SHMINDEX 1111
#define SM_NAME1 "sema1"
#define SM_NAME2 "sema2"
#define THOUSAND 1000
#define BILLION 1000000L

/* ( (([coef]*x-[value])^root)  ) */
typedef struct Poly
{
	int *coefs;
	int size;
}poly_t;

typedef struct myfoo
{
	double pow;
	poly_t poly;
}myfoo_t;

/* global vars */
sem_t mutex;
sem_t *sem1;
sem_t *sem2;
pthread_t* threads;
double *wheel;
int number_of_segment;
int *message;
int *counter;
FILE* logfile;
struct timespec start;

/* prototype for thread routine */
void handler ( void *ptr );
void print_usage_client();
double f(double val, myfoo_t*);
void calcel_threads();
char* get_foo();
myfoo_t parser();
int isConstant(const char*);
int base(int value);
int poly_create(const char *, myfoo_t*);
double compute_power(const char *);
void new_alloc(int power, int bigger, int**);

int number_of_threads;

/* flag for signal handling */
volatile sig_atomic_t die_flag = 0;

/* This static function to handle specified signal */
static void signal_func(int signo) {
    /* The signal flag */    
    die_flag = 1;
    printf("singal has caught! Please wait....\n");    
    
    calcel_threads();
}

main(int argc, char const *argv[])
{
	int shm_msg_id, shmid, shm_index_id;
	char logname[MAX_LENGHT];
	myfoo_t foo;
	int i;
	



	if(argc != 2){
		printf("Invalid operation!\n");
		print_usage_client();
	}	

	/* Signal handling */
    if (signal(SIGINT, signal_func) == SIG_ERR) {
        fputs("An error occurred while setting a signal handler.\n", stderr);
        exit( EXIT_FAILURE);
   	}

	number_of_threads = atoi(argv[1]);
	threads = (pthread_t*)malloc(number_of_threads*sizeof(pthread_t));
	sprintf(logname, "%s%d", "mclient_log",getpid()%100);
	logfile = fopen(logname, "w");

	/*	Load the function from file	*/
	foo = parser();

	/********************-Client logfile-**********************************/
	clock_gettime(CLOCK_REALTIME, &start);  
	fprintf(logfile, "Client started at %ld sec.\n", (start.tv_sec + start.tv_nsec/BILLION) );
	fprintf(logfile, "Thread id\t\t\t\t     x     \t\t\t\t   f(x)   \n");
	fprintf(logfile, "---------\t\t\t\t-----------\t\t\t\t----------\n");

	/********************** Semaphore initialize ****************************/
 	if ((sem1 = sem_open(SM_NAME1, O_RDWR)) == (sem_t *) -1) {
		perror("sem1:");
		exit(1);
	}
	if ((sem2 = sem_open(SM_NAME2, O_RDWR)) == (sem_t *) -1) {
		perror("sem2:");
		exit(1);
	}


	/************************************************************************/
	/******************** Shared memory (Message part) **********************/
	if ((shm_msg_id = shmget(SHMMSGKEY, sizeof(int), 0666)) < 0) {
		perror("shmget");
		return 1;
	}
	if ((message = shmat(shm_msg_id, NULL, 0)) < 0) {
		perror("shmat");
		return 1;
	}
	if ((shm_index_id = shmget(SHMINDEX, sizeof(int), 0666)) < 0) {
		perror("shmget");
		return 1;
	}
	if ((counter = shmat(shm_index_id, NULL, 0)) < 0) {
		perror("shmat");
		return 1;
	}
	/*************************************************************************/

	if(*message == 0){
		printf("Server is not active!\n");
		sem_destroy(&mutex); /* destroy semaphore */

		if(shmdt(message) != 0)
			fprintf(stderr, "Could not close memory segment.\n");

		free(threads);
 		exit(EXIT_FAILURE);
	}
	/***************************************************/	
	// printf("waiting..\n");
	// sem_wait(sem1);	
	// printf("in message critical \n");
	number_of_segment = *message;
	// sem_post(sem1); 
	// printf("out message critical \n");
	/***************************************************/

	printf("number_of_segment: %d\n", number_of_segment);

 	/******************** Shared memory (wheel part) **********************/
	if ((shmid = shmget(SHMKEY, number_of_segment*sizeof(double), 0666)) < 0) {
		perror("shmget");
		return 1;
	}
 
	if ((wheel = shmat(shmid, NULL, 0)) < 0) {
		perror("shmat");
		return 1;
	}
	/*************************************************************************/
	
	/************************** Thread Part ***********************************/
	sem_init(&mutex, 0, 1);
	sem_init(sem2, 0, 1);

	for (i = 0; i < number_of_threads; ++i){
        pthread_create (&threads[i], NULL, (void *) &handler, &foo);
    }                            

    for (i = 0; i < number_of_threads; ++i){
        pthread_join(threads[i], NULL);
    }

    sem_destroy(&mutex); /* destroy semaphore */

	// printf("mutex destroyed\n");
	/**************************************************************************/

	/************************-Free Part-***************************************/
	if(shmdt(wheel) != 0)
		fprintf(stderr, "Could not close memory segment.\n");
 	if(shmdt(message) != 0)
		fprintf(stderr, "Could not close memory segment.\n");
 	if(shmdt(counter) != 0)
		fprintf(stderr, "Could not close memory segment.\n");
 	
 	sem_close(sem1);
 	sem_close(sem2);

 	free(threads);
 	free(foo.poly.coefs);
 	fclose(logfile);

	return 0;
}
void handler ( void * ptr ){
	// printf("Thread: %d in handler\n", (int)pthread_self());
	struct timespec current;
	myfoo_t *foo = (myfoo_t*) ptr;

	while(!die_flag){
		if(sem1->__align == 0){			
			printf("Thread blocked because of Server is in critical section!\n", (unsigned int)pthread_self());				
			continue;			
		}else{
			if( sem_trywait(sem2) < 0){
				continue;
			}
			//printf("Thread: %d waiting for mutex\n", (int)pthread_self());; 
			sem_wait(&mutex); 
			printf("Thread: %u in critical section\n", (unsigned int)pthread_self());
			// printf("x = %0.2f, f(x) = %0.2f\n", wheel[counter], f(wheel[counter]));
			fprintf(logfile, "%u\t\t\t\t  %.2f\t\t\t\t  %.2f\n", (unsigned int)pthread_self(), wheel[*counter], f(wheel[*counter], foo));
			if(*message == 0){				
				// pthread_exit(NULL);
				printf("server terminated!\n");
				fprintf(logfile, "server terminated. Thread %u exited\n", (unsigned int)pthread_self());
				sem_post(&mutex);
				sem_post(sem2);
				break;
			}
			wheel[*counter] = f(wheel[*counter], foo);
			// printf("bu yazıldı %f\n", wheel[*counter]);
			(*counter) += 1;
			//printf("counter:%d\n", counter);
			if((*counter) == number_of_segment)
				*counter = 0;			
			sem_post(&mutex);
			sem_post(sem2);
			// printf("Thread: %u out critical section\n", (unsigned int)pthread_self());
		}					
	}
}
double f(double val, myfoo_t* foo){
	int size = foo->poly.size;
	int *coefs = foo->poly.coefs;
	double power =  foo->pow;
	double fnc = 0;	
	double sum = 0;
	int i;

	for(i = 0; i < size; ++i){
		//printf("val: %f, coefs[%d]: %d\n", val, i, coefs[i]);
		sum += coefs[i]*pow(val, i);
	} 
	//printf("sum %f\n", sum);
	//sleep(5);
	fnc = pow(sum, foo->pow);
	//sleep(5);
	return fnc;
}
void print_usage_client(){
	printf("Usage: ./mclient <#threads>\n");
	printf("Parsing standart: (<function>)^(<power>)\n");
	printf("Example: (x^2)^(1/3)\n");	
	exit(-1);
}
void calcel_threads(){
	int i;	
	for (i = 0; i < number_of_threads; ++i){
    	pthread_cancel(threads[i]);
    }    
}
myfoo_t parser(){
	int index = 0, lenght;
	int value;
	double power;
	char* getline;
	char line[MAX_LENGHT];
	myfoo_t foo_t;

	getline = get_foo();
	// printf("line %s\n", getline);
	strcpy(line, getline);
	free(getline);

	lenght = strlen(line);
	
	// printf("index %d, lenght:%d \n", index, lenght);
	if(line[index] != '('){
		printf("missing parantesis\n");
		exit(EXIT_FAILURE);
	}
	++index;
	/*Buraya kadar ilk parantez garanti */
	
	index += poly_create(&line[index], &foo_t); /* Parantezler arası polinomu hesaplayan kısım */
	// printf("son index %d, son karakter :%c\n", index, line[index]);
	/* Buraya kadar index son parantezi göstermeli.
	   index+1 '^' karakterini göstermeli
	*/
	if(line[index] == ')' && line[index+1] == '^'){
		index += 2;
	}else{
		printf("syntax error: invalid character '%c' or '%c'\n", line[index], line[index+1]);
	}

	/* Buraya kadar polinom, parantezler ve üs işareti garantilendi.
		Gerisi üs hesaplama
	*/	

	/* Üs hesaplandı. */
	power = compute_power(&line[index]);
	foo_t.pow = power;
	// printf("power: %f\n", power);
	// printf("bitti\n");
	return foo_t;
}

double compute_power(const char* line){
	int value, value2;
	int index = 0;
	int paranflag = 0;
	char operator;

	if(line[index] == '('){
		paranflag = 1;
		++index;
	}
		
	value = isConstant(&line[index]);
	
	// printf("index: %d\n", index);
	if(value == 0)
		index += 1;
	else	 
		index += base(value);

	// printf("index2: %d\n", index);
	operator = line[index++];
	// printf("index3: %d\n", index);
	// printf("operator: %c\n", operator);
	
	value2 = isConstant(&line[index]);
	
	if(value2 == -1){
		printf("Syntax error after '%c'\n", operator);
		printf("Parsing standart: (<function>)^(<power>)\n");
		exit(EXIT_FAILURE);
	}
	else if(value2 == 0)
		index += 1;
	else	 
		index += base(value2);

	if(paranflag){
		if(line[index] != ')'){
			printf("missing parantesis! '%c'\n", line[index]);
			printf("Parsing standart: (<function>)^(<power>)\n");
			exit(EXIT_FAILURE);
		}
	}
	if(operator == '/')
		return (double)value/value2;
	else if(operator == '.'){
		return value + (value2*0.1);
	}else{
		printf("Unknown operator %c\n", operator);
		printf("Parsing standart: (<function>)^(<power>)\n");

		exit(EXIT_FAILURE);
	}
}
int poly_create(const char *line, myfoo_t* foo_t){
	// printf("%s\n", line);
	
	char c;
	int index = 0;
	int lastOp = -1;
	int theCoef = 1;
	int power;
	int bigger = 0;
	int i;

	while(line[index] != ')' ){
		// printf("line[%d]: %c\n", index, line[index]);
		if(line[0] != 'x' && !(line[0] > 47 && line[0] < 58)){
			printf("syntax error 1! Invalid character: %c \n", line[index]);
			printf("Parsing standart: (<function>)^(<power>)\n");
			exit(EXIT_FAILURE);
		}
		if(line[index] == 'x'){
			if(line[index+1] == '^' && ((power = isConstant(&line[index+2])) > 0) ){
				if(power > bigger){ 
					// printf("flag1\n");
					new_alloc(power, bigger, &foo_t->poly.coefs);
					bigger = power;							
					//foo_t->poly.coefs = (int*)calloc(bigger+1, sizeof(int*));
				}
				//printf("geldi\n");
				// printf("new_coefs[%d]: %d\n", power, foo_t->poly.coefs[power]);
				if(line[index-1] == '-')
					foo_t->poly.coefs[power] = 1;
				else
					foo_t->poly.coefs[power] = theCoef;
				//printf("gitti\n");				
				index = index + 2 + base(theCoef);
			}else if(line[index+1] == '^' && power <= 0){
				printf("Bir hata(3) var: nearby: %c\n", line[index+2]);
				printf("Parsing standart: (<function>)^(<power>)\n");
				exit(EXIT_FAILURE);	
			}else if(line[index+1] != '+' && line[index+1] != '-' && line[index+1] != '*' && line[index+1] != '/' && line[index+1] != ')' ){
				printf("Bir hata(4) var: nearby: %c\n", line[index+1]);
				printf("Parsing standart: (<function>)^(<power>)\n");
				exit(EXIT_FAILURE);	
			}else{
				if(bigger == 0){
					bigger = 1;
					foo_t->poly.coefs = (int*)calloc(bigger, sizeof(int*));
				}
				if(line[index-1] == '-')
					foo_t->poly.coefs[1] = -1;
				else{
					if(line[index-1] == '-' || line[index-1] == '+')
						foo_t->poly.coefs[1] = 1;
					else
						foo_t->poly.coefs[1] = theCoef;
				}
					
				// printf("yanlız x theCoef %d, power\n", theCoef);
				// printf("coefs[1]: %d\n", foo_t->poly.coefs[1]);
				++index;					
			}

		}else if(line[index] == '^'){
			printf("Bir hata var(5): nearby: %c\n", line[index]);
			printf("Parsing standart: (<function>)^(<power>)\n");
			exit(EXIT_FAILURE);	
		}else if(line[index] == '*'){
			// printf("flag8\n");
			if( !( (line[index-1] > 47 && line[index-1] < 58) && (line[index+1] == 'x') ) ){
				printf("Bir hata var(6): before or next of: %c\n", line[index]);
				printf("Parsing standart: (<function>)^(<power>)\n");
				exit(EXIT_FAILURE);	
			}else
				index++;			
		}else if(line[index] == '+' || line[index] == '-'){
			if( (line[index+1] != 'x' && !(line[index+1] > 47 && (line[index+1] < 58))) 
			 || (line[index-1] != 'x' && !(line[index-1] > 47 && (line[index-1] < 58))) ){
				printf("Bir hata var(7): nearby: %c\n", line[index]);
			printf("Parsing standart: (<function>)^(<power>)\n");
				exit(EXIT_FAILURE);	
			}
			++index;
		}else if(line[index] > 47 && line[index] < 58){
			theCoef = isConstant(&line[index]); //isConstanta dikkat.
			// printf("theCoef %d\n", theCoef);
			if(index != 0){
				if(line[index-1] == '-'){
					theCoef = -theCoef;
				}else if(line[index-1] != '+'){
					printf("Error(8). Unknown character %c\n", line[index-1]);
					printf("Parsing standart: (<function>)^(<power>)\n");
					exit(EXIT_FAILURE);
				}
			}
			index += base(theCoef);
			if(line[index] == '-'){
				if(bigger == 0){
					bigger = 1;
					foo_t->poly.coefs = (int*)calloc(bigger, sizeof(int*));
				}
				foo_t->poly.coefs[0] = theCoef;
				// printf("coefs[0]: %d\n", foo_t->poly.coefs[0]);
			}
			if (line[index] == ')'){
				if(bigger == 0){
					bigger = 1;
					foo_t->poly.coefs = (int*)calloc(bigger, sizeof(int*));
				}
				foo_t->poly.coefs[0] = theCoef;
				// printf("coefs[0]: %d\n", foo_t->poly.coefs[0]);
			}
		}else if(line[index] != '+' && line[index] != '-' && line[index] != ')'){				
			printf("Error(9). Unknown character %c\n", line[index-1]);
			printf("Parsing standart: (<function>)^(<power>)\n");
			exit(EXIT_FAILURE);					
		}
	}

	for(i = 0; i <= bigger; i++ )
	{
		printf("coefs[%d] = %d\n", i, foo_t->poly.coefs[i]);
	}

	foo_t->poly.size = bigger + 1;
	return index;
}

void new_alloc(int power, int bigger, int** coefs){
	int* new_coefs;
	int i = 0;
	// printf("power %d, bigger %d\n", power, bigger);
	new_coefs = calloc(power+1, sizeof(int*));

	printf("new alloc\n");
	for (i = 0; i < bigger; ++i){
		new_coefs[i] = *coefs[i];
		printf("new_coefs[%d]: %d\n", i, new_coefs[i]);
	}

	if(bigger != 0)
		free(*coefs);

	*coefs = new_coefs;

}
char* get_foo(){
	FILE* fp = fopen("function.dat","r");
	char *line = NULL;
	size_t nval;

	getline(&line, &nval, fp);

	// free(line);
	fclose(fp);
	return line;
}
int isConstant(const char* expr){
	int res;
	if( expr[0] == '*' || expr[0] == '+' || expr[0] == '-' || expr[0] == '/' || expr[0] == '.' || 
		expr[0] == '\0' || expr[0] == ')' || expr[0] == '0'){
		return 0;
	}else if(expr[0] > 57 || expr[0] < 48){
		return -1;
	}

	res = isConstant(&expr[1]);
	if(res == 0)
		return ((int)expr[0]-48);
	else if(res == -1)
		return -1;
	else
		return 10*((int)expr[0]-48) + res;
}
int base(int value){
	
	if(value == 0)
		return 0;
	else
		return 1 + base(value/10);
}
