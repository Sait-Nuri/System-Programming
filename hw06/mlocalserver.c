#include <sys/stat.h>
#include <string.h>
#include <termios.h>
#include <errno.h>
#include <termios.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <math.h>

#define MAX_LENGHT 512
#define SHMKEY 1234
#define SHMMSGKEY 4321
#define SHMINDEX 1111
#define SM_NAME1 "sema1"
#define SM_NAME2 "sema2"
#define THOUSAND 1000
#define BILLION 1000000000000L
#define MILLION 1000000
/* ( (([coef]*x-[value])^root)  ) */
typedef struct Poly
{
	int *coefs;
	int size;
}poly_t;

typedef struct myfoo
{
	double pow;
	int global_index;
	poly_t poly;
}myfoo_t;

pthread_t* threads;
struct timespec start;
sem_t mutex;
sem_t *sem1;
int *message;
int number_of_segment;
int number_of_threads;
volatile int counter;
double* wheel;

void print_usage_localserver();
void handler ( void *ptr );
double f(double val, myfoo_t*);
void calcel_threads();
char* get_foo();
myfoo_t parser();
int isConstant(const char*);
int base(int value);
int poly_create(const char *, myfoo_t*);
double compute_power(const char *);
void new_alloc(int power, int bigger, int**);
double get_avarage(double*, int);
void print_segment(double*, int);
void reporting(double , FILE* );

/* flag for signal handling */
volatile sig_atomic_t die_flag = 0;

/* This static function to handle specified signal */
static void signal_func(int signo) {
    
    /* The signal flag */    
    printf("singal has caught! Please wait....\n");    
    die_flag = 1;
    
}

int main(int argc, char const *argv[])
{
	FILE * logfile;	
	char *shm;
	int buffer_size;
	int sampling_time;
	int size;
	int shmid, shm_index, shm_msg_id;
	int i;
	double avarage;
	myfoo_t foo;
	sem_t *sem2;

	if(argc != 4){
		printf("invalid operation!\n");
		print_usage_localserver();
	}

	sem_unlink(SM_NAME1);
	sem_unlink(SM_NAME2);

	/* initialize logfile */	
	if( (logfile = fopen("mlocalserver_log", "w")) < 0){
		perror("logfile:");
		exit(EXIT_FAILURE);
	}

	/* Signal handling */
    if (signal(SIGINT, signal_func) == SIG_ERR) {
        fputs("An error occurred while setting a signal handler.\n", stderr);
        exit( EXIT_FAILURE);
   	}

   	clock_gettime(CLOCK_REALTIME, &start);  
	fprintf(logfile, "Local Server started at %ld sec.\n", (start.tv_sec + start.tv_nsec/BILLION) );
	fprintf(logfile, "Avarage\t\t\t\t\tTime\n");
	fprintf(logfile, "-------\t\t\t\t------------\n");

	/* initialize the buffer size */
	buffer_size = atoi(argv[1]);	

	/* Obtain the thread number */
	number_of_threads = atoi(argv[2]);

	/* Create new threads */
	threads = (pthread_t*)malloc(number_of_threads*sizeof(pthread_t));
	// printf("number_of_threads %d\n", number_of_threads);
	
	/* initialize the sampling time */
	sampling_time = atoi(argv[3]);	

	/************************** Semaphore and Shared memory *************************************/
    /* Protection for shared memory */
    if ((sem1 = sem_open(SM_NAME1, O_CREAT, 0666, 1)) < 0) {
		perror("sem1:");
		exit(1);
	}
	
	if ((sem2 = sem_open(SM_NAME2, O_CREAT, 0666, 0)) < 0) {
		perror("sem2:");
		exit(1);
	}

	/* Shared memory for status */
	if ((shm_msg_id = shmget(SHMMSGKEY, sizeof(int), IPC_CREAT | 0666)) < 0){
 		perror("shmget");
		return 1;	
 	}

 	if ((message = shmat(shm_msg_id, NULL, 0)) < 0) {
		perror("shmat");
		return 1;
	}
	*message = 1;
	/* Shared memory for wheel of fortune */
	if ((shmid = shmget(SHMKEY, buffer_size, IPC_CREAT | 0666)) < 0) {
		perror("shmget");
		return 1;
	}
 	
	if ((shm = shmat(shmid, NULL, 0)) < 0) {
		perror("shmat");
		return 1;
	}

	/***************************************************************************/

	/* Casting wheel of fortune to double array */
	wheel = (double*)shm;
	
	/* Determine the size of double array */
	size = (buffer_size / sizeof(double));	
	number_of_segment = size;
	/******************* -initialize wheel- *******************/
	for(i = 0; i < size; i++){		
		wheel[i] = (1.0/size)*i;
	}
	/***********************************************/

	/*	Load the function from file	*/
	foo = parser();
	foo.global_index = 0;
	/************************** Thread Part ***********************************/
	sem_init(&mutex, 0, 1);

	for (i = 0; i < number_of_threads; ++i){
        pthread_create (&threads[i], NULL, (void *) &handler, &foo);
    }                            
	/**************************************************************************/

    /**************** -Avarage Part- ************************/	
	while(!die_flag){		
		usleep(sampling_time*THOUSAND);	
		sem_wait(sem1);						//printf("in critical sem1: %d\n", (int)sem1->__align);			
		avarage = get_avarage(wheel, size);
		printf("avarage: %f\n", avarage);	
		//print_segment(wheel, size);	
		srand(time(NULL));
		counter = rand() % size;
		reporting( avarage, logfile );
		sem_post(sem1);						//printf("out of critical sem1: %d\n\n", (int)sem1->__align);
	}		
	/*************************************************/

    if(die_flag){
		//printf("mesaj görüldü\n");
		*message = 0;
		fprintf(logfile, "Server terminated by user\n" );
	}

	for (i = 0; i < number_of_threads; ++i){
        pthread_join(threads[i], NULL);
    }

 //    for(i = 0; i <= foo.poly.size; i++ )
	// {
	// 	printf("coefs[%d] = %d\n", i, foo.poly.coefs[i]);
	// }

    free(foo.poly.coefs);
    free(threads);

    sem_destroy(&mutex); /* destroy semaphore */
	// printf("mutex destroyed\n");

	if(shmdt(shm) != 0)
		fprintf(stderr, "Could not close memory segment.\n");
	
	if(shmdt(message) != 0)
		fprintf(stderr, "Could not close memory segment.\n");
	 
 	sem_close(sem1);
 	sem_close(sem2);
 	fclose(logfile);

	return 0;
}
void handler ( void * ptr ){
	// printf("Thread: %d in handler\n", (int)pthread_self());
	FILE* logptr;
	struct timespec current;
	myfoo_t *foo = (myfoo_t*) ptr;
	double sec;
	char thread_id[MAX_LENGHT];
	int i;
	double sum = 0;

	sprintf(thread_id, "%s%u%s", "thread_", (unsigned int)pthread_self(), "_log");

	/* initialize logfile */	
	if( (logptr = fopen(thread_id, "w")) < 0){
		perror("logptr:");
		exit(EXIT_FAILURE);
	}	

	clock_gettime(CLOCK_REALTIME, &current);  
	fprintf(logptr, "Thread %u started at %ld millisec.\n", (unsigned int)pthread_self(), (current.tv_sec)*THOUSAND + (current.tv_nsec/MILLION) );
	fprintf(logptr, "Time(ms)\t\t\t\t     x     \t\t\t\t   f(x)   \n");
	fprintf(logptr, "---------\t\t\t\t-----------\t\t\t\t----------\n");
	
	while(!die_flag){

		if(sem1->__align == 0){			
			printf("Thread %u blocked because of Server is in critical section!\n", (unsigned int)pthread_self());			
			continue;			
		}else{
			
			sem_wait(&mutex);  
			
			clock_gettime(CLOCK_REALTIME, &current); 
			sec = (current.tv_sec - start.tv_sec)*THOUSAND + (current.tv_nsec-start.tv_nsec)/MILLION;
		
			fprintf(logptr, "  %.2f \t\t\t\t\t %.2f\t\t\t\t\t %.2f\n", sec, wheel[counter],  f(wheel[counter], foo));
			if(*message == 0){				
				// calcel_threads();
				printf("local server terminated!\n");
				fprintf(logptr, "local server terminated. Thread %u exited\n", (unsigned int)pthread_self());
				sem_post(&mutex);
				break;
			}
	
			wheel[counter] = f(wheel[counter], foo);

			++counter;
		
			if((counter) == number_of_segment){
				counter = 0;
			}
							
			sem_post(&mutex);
		}					
	}
	fclose(logptr);
	pthread_exit(NULL);
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
	// sleep(3);
	// printf("sum %f\n", sum);
	fnc = pow(sum, foo->pow);
	
	return fnc;
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
					new_alloc(power, bigger, &foo_t->poly.coefs);
					bigger = power;							
				}
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

	// printf("poly create\n");
	// for(i = 0; i <= bigger; i++ )
	// {
	// 	printf("coefs[%d] = %d\n", i, foo_t->poly.coefs[i]);
	// }

	foo_t->poly.size = bigger + 1;

	// printf("poly create\n");
	// for(i = 0; i < foo_t->poly.size; i++ )
	// {
	// 	printf("coefs[%d] = %d\n", i, foo_t->poly.coefs[i]);
	// }
	return index;
}

void new_alloc(int power, int bigger, int** coefs){
	int* new_coefs;
	int i = 0;
	// printf("power %d, bigger %d\n", power, bigger);
	new_coefs = calloc(power+1, sizeof(int*));

	for (i = 0; i < bigger; ++i){
		new_coefs[i] = *coefs[i];
		//printf("new_coefs[%d]: %d\n", i, new_coefs[i]);
	}
	// printf("free\n");
	if(bigger != 0)
		free(*coefs);
	// printf("free\n");

	*coefs = new_coefs;
}
char* get_foo(){
	FILE* fp = fopen("function.dat","r");
	char *line = NULL;
	size_t nval;

	getline(&line, &nval, fp);

	//free(line);
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

	if(expr[1] == '0')
		return 10*((int)expr[0]-48);

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
	else{
		return 1 + base(value/10);
	}
		
}

void print_usage_localserver(){
	printf("Usage: ./mserver <buffer_size> <#thread(s)> <update_time [millisec]>\n");
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
double get_avarage(double* wheel, int size){
	double sum = 0;
	int i;

	for (i = 0; i < size; ++i){
		sum += wheel[i];
	}

	// printf("avarage: %f\n", sum/size);
	return sum/size;
}

void reporting(double avarage, FILE* logfile){
	struct timespec current;
	clock_gettime(CLOCK_REALTIME, &current);   
	fprintf(logfile, "%.2f\t\t\t\t%ld millisec.\n", avarage, ( (current.tv_sec-start.tv_sec)*THOUSAND + 
		(current.tv_nsec-start.tv_nsec)/MILLION ));
}

void print_segment(double* segment, int size){
	int i;

	for(i = 0; i < size; i++){			
		printf("%0.2f\n", segment[i]);	
	}
}