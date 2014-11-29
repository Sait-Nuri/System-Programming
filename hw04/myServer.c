/*
	----------------------USAGE------------------------------
	| > make                                                |
	| > ./server "polinomial function" [sample_time]        |
	|                                                       |
	|  EXAMPLE1: ./server "x^2+3*x+sin(3*x)" 1000000000     |
	|  EXAMPLE2: ./server "4*x^3+3*x^2+cos(x^2)" 100000000  |	
	|                                                       | 
	---------------------------------------------------------
*/

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <termios.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

#define BILLION 1000000000L
#define MILLION 1000000L
#define THOUSAND 1000L
#define MY_FIFO "/tmp/"
#define LOGNAME "serverLog"
#define MAX_PATH 512
#define MAX_TIME 20
#define PID_LENGNT 6
#define H 100000
#define PI 3.14159265
#define SIZE 10

/* Keeps information about a client */
struct clientData
{
	int clientType;
	char clientPid[PID_LENGNT];
	char client_life_str[MAX_TIME];

};

/* The Data about server due to send them to thread */
struct sentData
{
	double result;
	long connection_time;
	long elapsed_time;
	int status;
};

/* Structure for polinomial values  */
struct Poly_t
{
	int *coefs;
	int size;
};

/* Structure for sinus values 	*/
struct Sin_t{
	int *coefs;
	int size;
};

/* Structure for cosinus values 	*/
struct Cos_t
{
	int *coefs;	
	int size;
};

/* Structure for exponential values 	*/
struct Exp_t
{
	int *coefs;	
	int size;
};

/* Structure for keeps some values to send them to thread 	*/
typedef struct moveData
{
	struct timespec begin;				/* Keeps beginning time */
	struct clientData client;			/* Client data */
	long lSsr;							/* Server sending frequnecy */
	FILE* fpLogfile;					
	struct Poly_t poly;
	struct Sin_t sinus;
	struct Cos_t cosinus;
	struct Exp_t expon;
	double (*deriv)(struct timespec, struct Poly_t*, struct Sin_t*, 
				struct Cos_t*, struct Exp_t*);
	double (*integ)(struct timespec, struct timespec,  struct Poly_t*, 
				struct Sin_t*, struct Cos_t*, struct Exp_t*);
}moveData_t;

double getInteg(struct timespec , struct timespec,  struct Poly_t* , struct Sin_t* , struct Cos_t* , struct Exp_t*);
double getDeriv(struct timespec , struct Poly_t* , struct Sin_t* , struct Cos_t* , struct Exp_t* );
double evaluate(double, struct Poly_t* , struct Sin_t* , struct Cos_t* , struct Exp_t* );
void initLogFile(FILE*);
int findCoef(const char*, int, int);
int findExpo(const char*, int, int*);
int allocation(const char*, int , int , int*);
void sinOperation(const char*, int , int*);
void cosOperation(const char*, int , int*);
void expOperation(const char*, int , int*);
int isConstant(const char*);
void initialize(const char*, int, int, int*);
void getValues(struct Poly_t* poly, struct Sin_t* sinus, struct Cos_t* cosinus, struct Exp_t* expon);
void print_usage();

/* flag for signal handling */
volatile sig_atomic_t die_flag = 0; 

/* This static function to handle specified signal */
static void signal_func(int signo) {
        
    /* The signal flag */    
    die_flag = 1;
    printf("singal has caught! Please wait....\n");    
}

/* Thread argumant function */
void *threadFunc(void *rcv)
{
	int iClientfd;								/* File descriptor of the client 		 */
	int iStatusfd;								/* Status flag 							 */
	int iFirstFlag = 1;							/* First condition flag 				 */
	int iPlusFlag = 1;							/* First spotted plus flag 				 */
	int iAllocFlag = 1;							/* Allocation flag 						 */
	int iControlFlag = 1;						/* A control flag 						 */
	int iCoef, iExpo;							/* Values of coeficiants and exponantial */
	int iIndexPlus = 0;							/* Index of '+' character 				 */
	int iTime_of_round = 0;						/* Quentity of the looping 				 */
	int tmr = 0;
	long lElapsed;								/* Elapsed time  						 */
	long lDelay;								/* Delay time 							 */
	long lServer_send_rate;						/* Server data sending frequency 		 */	
	char chClient_life[MAX_TIME];				/* Quentity of client life time  		 */
	char chClient_pid[PID_LENGNT];				/* Client's pid 						 */
	char chChild_fifo_path[MAX_PATH];			/* Path to fifo of the client 			 */
	char chValid;								/* Validation character 				 */
	struct sentData sentData1;					/* A data structure to send 			 */
	struct timespec begin, finish, prev;		/* Keep time values 					 */
	struct timespec server_timer;				/* Server timer 						 */
	struct clientData client;					/* Client's data 						 */
	FILE* fpLogfile;							/* Log file of the server 				 */
	moveData_t * data;							/* Data sent to thread 					 */
	struct Poly_t poly;							
	struct Sin_t sinus;							
	struct Cos_t cosinus;						
	struct Exp_t expon;							

	/* Signal handling */
    if (signal(SIGINT, signal_func) == SIG_ERR) {
        fputs("An error occurred while setting a signal handler.\n", stderr);
        exit( EXIT_FAILURE);
   	}

	data = rcv;
	begin = data->begin;
	client = data->client;
	lServer_send_rate = data->lSsr;
	fpLogfile = data->fpLogfile;
	poly = data->poly;
	sinus = data->sinus;
	cosinus = data->cosinus;
	expon = data->expon;

	// printf("client type %d\n", client.clientType);
	/* Initialize the conection time of client */
   	sentData1.connection_time = (begin.tv_sec*THOUSAND + begin.tv_nsec/MILLION);	
	
	strcpy(chClient_life ,client.client_life_str);
	strcpy(chClient_pid, client.clientPid);
	
	//printf("lSsr %ld\n", lServer_send_rate);
	iTime_of_round = (atol(chClient_life)*THOUSAND)/(lServer_send_rate/1000); //=> mikro sec
	tmr = iTime_of_round;
	sprintf(chChild_fifo_path, "%s%s", MY_FIFO, chClient_pid); 
	//printf("my child fifo path: %s \n", chChild_fifo_path);			

	server_timer.tv_sec = lServer_send_rate/BILLION;
	server_timer.tv_nsec = lServer_send_rate - (server_timer.tv_sec*BILLION );

	/***************** Create fifo for each client ****************/
	if( mkfifo(chChild_fifo_path, 0666) < 0){
		perror("fifo not created:");
		exit(EXIT_FAILURE);	
	}
	
	//printf("child_fifo oluştu \n");
	/* Client fifo açıldı */
	if( (iClientfd = open(chChild_fifo_path, O_WRONLY)) < 0){
		perror("open fifo");
		exit(EXIT_FAILURE);
	}
	//printf("iClientfd açıldı. %d\n", iClientfd);
	/*************************************************************/

	/**************Create status fifo for client *****************/
	strcat(chChild_fifo_path, "status");	
	//printf("status path %s\n", chChild_fifo_path);

	if( mkfifo(chChild_fifo_path, 0666) < 0){
		perror("fifo not created:");
		exit(EXIT_FAILURE);	
	}

	if( (iStatusfd = open(chChild_fifo_path, O_RDONLY)) < 0){
		perror("open fifo");
		exit(EXIT_FAILURE);
	}
	/*************************************************************/
	//printf("status fifo oluştu\n");

	printf("My thread id: %lu\n", (pthread_t)pthread_self());
	sentData1.status = 1;
	
	clock_gettime(CLOCK_REALTIME,&prev);								
	prev.tv_sec -= server_timer.tv_sec;
	prev.tv_nsec -= server_timer.tv_nsec;

	// printf("tmr %d\n", tmr);

	while(iTime_of_round--)
	{
		if(die_flag){			
			sentData1.status = 0;
			write(iClientfd, (void*)&sentData1, sizeof(struct sentData));
			fprintf(fpLogfile, "Ctrl+c detected!\n");					
			fprintf(fpLogfile, "Childs are notified and killed\n");
			fflush(fpLogfile);
			break;
		}
		
		clock_gettime(CLOCK_REALTIME,&finish);								

		/* Stub function of derivative always returns 1 */
		if(client.clientType == 1)
			sentData1.result = data->deriv(finish, &poly, &sinus, &cosinus, &expon); //=>
		else if(client.clientType == 2)
			sentData1.result = data->integ(finish, prev, &poly, &sinus, &cosinus, &expon);	 //=>		

		prev = finish;

		/* Send current micro second */
		sentData1.elapsed_time =  ((finish.tv_sec - begin.tv_sec)*MILLION) + (finish.tv_nsec - begin.tv_nsec)/1000;

		write(iClientfd, (void*)&sentData1, sizeof(struct sentData));
		/* Check whether client gets an iterrupt or not*/

		tmr = iTime_of_round;
		read(iStatusfd, &chValid, 2);
		iTime_of_round = tmr;

		if(chValid == '0'){			
			fprintf(fpLogfile,"client %s got ctrl+c signal and died\n", chClient_pid);
			fflush(fpLogfile);
			break;
		}				
		nanosleep(&server_timer, NULL);
	}

	printf("writing is done\n");
	close(iClientfd);

	pthread_exit(NULL);
}

int main(int argc, char const *argv[])
{
	const char* chpExpression = argv[1];
	int iServerfd;					/* File descriptor for Server 								*/
	int iCur_index = 0;				/* Keeps index value for a loop 							*/
	int exprSize;					/* Keeps lenght of the initial time arguman from argv 		*/
	int start, end;					/* Keeps the index of some charachter 						*/
	int i = 0;						/* An index value 											*/
	long lServer_send_rate;			/* Data sending frequency of the server 					*/
	char server_fifo_path[MAX_PATH];/* Path to fifo of the server 								*/	
	struct Poly_t poly;				/* Structure for polinomial values  						*/
	struct Sin_t sinus;				/* Structure for sinus values 								*/
	struct Cos_t cosinus;			/* Structure for cosinus values 							*/
	struct Exp_t expon;				/* Structure for exponantial values 						*/
	struct timespec begin;			/* Keeps starting time values 								*/
	struct clientData client1;		/* Keeps some information of client 						*/
	pid_t pid;						/* PID of the Server 									    */
	pthread_t cur_thread;			/* Thread to handle a client 								*/
	FILE* fpLogfile;				/* Server's logfile descriptor  							*/
	moveData_t data;				/* Structure for keeps some values to send them to thread 	*/

	if(argc != 3){
		if(argc < 3)
			printf("Missing argument!\n");
		else
			printf("Too many arguments!\n");
		print_usage();
		exit(EXIT_FAILURE);
	}

	chpExpression = argv[1];
	exprSize = strlen(chpExpression);
   	start = iCur_index;

   	// printf("chpExpression: %s\n", chpExpression);
   	// printf("exprSize: %d\n", exprSize);   	

   	while( (iCur_index++) < exprSize){		
		/* Sin cos exp göresiye kadar ilerle */
		if(chpExpression[iCur_index] == 's' || chpExpression[iCur_index] == 'c' || chpExpression[iCur_index] == 'e'){			
			/* Allocation for polynome */
			end = iCur_index-1;
			
			/* Allocation of polynome */
			if( allocation(chpExpression, start, end, poly.coefs) == -1){				
				break;
			}
			
			/* if not sin, print error, exit operations */
			if( (chpExpression[iCur_index+1] == 'i' && chpExpression[iCur_index+2] == 'n')) {
				if(chpExpression[iCur_index+3] == '('){					
					iCur_index += 4; 
					sinOperation(chpExpression, iCur_index, sinus.coefs);
					break;
				}else{
					printf("Syntax error at %c\n", chpExpression[iCur_index+3]);
					break;
				}									
			}
			else if( (chpExpression[iCur_index+1] == 'o' && chpExpression[iCur_index+2] == 's')){
				if(chpExpression[iCur_index+3] == '('){
					iCur_index += 4; 
					cosOperation(chpExpression, iCur_index, cosinus.coefs);
					break;
				}else{
					printf("Syntax error at %c\n", chpExpression[iCur_index+3]);
				}				
			}
			else if( (chpExpression[iCur_index+1] == 'x' && chpExpression[iCur_index+2] == 'p')){
				if(chpExpression[iCur_index+3] == '('){
					iCur_index += 4; 
					expOperation(chpExpression, iCur_index, expon.coefs);
					break;
				}else{
					printf("Syntax error at %c\n", chpExpression[iCur_index+3]);
					break;
				}
			}else{
				printf("Parsing error: unknown character\nProgram exited");
				exit(EXIT_FAILURE);
			}
		}
	}	
	
	getValues(&poly, &sinus, &cosinus, &expon);
	//printf("sinus.coefs %d\n", sinus.coefs[0]);
	data.poly = poly;
	data.sinus = sinus;
	data.cosinus = cosinus;
	data.expon = expon;
	data.deriv = &getDeriv;
	data.integ = &getInteg;
   	/************************************************************************/


	/******************************* Server part *********************************/

	printf("Welcome to my server\nMy pid: %d\n", (int)getpid());

	/* Log file created */
   	if( (fpLogfile = fopen(LOGNAME, "w")) == NULL ){
   		perror("fopen");
   		exit(EXIT_FAILURE);
   	}
   	data.fpLogfile = fpLogfile;
   	/* Get server time */
   	clock_gettime(CLOCK_REALTIME, &begin);   	
   	//printf("sec: %ld\n", begin.tv_sec);
   	//printf("nsec:%ld\n", begin.tv_nsec);

	fprintf(fpLogfile, "Holy Math Server borns at %ld  second, %ld microsec.\n", begin.tv_sec, begin.tv_nsec/1000);
   	fflush(fpLogfile);
   	/********************* Server timer **********************************/
   	/* rate of sending data to clients */
   	lServer_send_rate = atol(argv[2]);
   	//printf("lServer_send_rate: %ld\n", lServer_send_rate);
   	data.lSsr = lServer_send_rate;

   	
	/**************** Fifo creating and openning **************/
	sprintf(server_fifo_path, "%s%d", MY_FIFO, getpid());	
	//printf("server_fifo_path: %s\n", server_fifo_path);
	
	/* Create and open fifo */
	if( mkfifo(server_fifo_path, 0666) < 0){
		perror("fifo not created:");
		exit(EXIT_FAILURE);	
	}

	if( (iServerfd = open(server_fifo_path, O_RDONLY)) < 0){
		perror("open fifo");
		exit(EXIT_FAILURE);
	}	
	
	while(1){
		/* Read client's message */	
		//fprintf(stdout, "Waiting for a new client... \n");	
		while(!die_flag){						
			if(read(iServerfd, (void*)&client1, sizeof(struct clientData) ) > 0)
				break;
			else if(errno == ENOENT){
				perror("read fifo");
				exit(EXIT_FAILURE);
			}
		}		

		if(die_flag){
		// 	fprintf(fpLogfile, "Ctrl+c detected!\n");
		// 	fflush(fpLogfile);
		break;
		}

		/* Get server time */
	   	clock_gettime(CLOCK_REALTIME, &begin); 
	   	data.begin = begin;
	   	data.client = client1;	   	
	   	fprintf(fpLogfile, "Client is connected (%s)\n", client1.clientPid);
		fflush(fpLogfile);		
	
		if (pthread_create(&cur_thread, NULL, threadFunc, (void *) &data) != 0){
			perror("thread create:");
			exit(EXIT_FAILURE);
		} 			
		/*************************************************/
	}		

	return 0;
}


double getDeriv(struct timespec curTime, struct Poly_t* poly, struct Sin_t* sinus, 
				struct Cos_t* cosinus, struct Exp_t* expon){
	double x_h;
	double x;

	// printf("curTime.tv_sec %lu\n", curTime.tv_sec);
	// Send millisec
	x_h = evaluate( (curTime.tv_sec) + (curTime.tv_nsec/BILLION) + H , poly, sinus, cosinus, expon);
	  x = evaluate( (curTime.tv_sec) + (curTime.tv_nsec/BILLION) , poly, sinus, cosinus, expon);

	return (x_h-x)/H;
}
double getInteg(struct timespec current, struct timespec prev,  struct Poly_t* poly, 
				struct Sin_t* sinus, struct Cos_t* cosinus, struct Exp_t* expon){
	double x_1, x;

	// printf("current %lu\n", current.tv_sec);
	// printf("prev %lu\n", prev.tv_sec);
	x_1 = evaluate( (current.tv_sec) + (current.tv_nsec/BILLION) , poly, sinus, cosinus, expon );
	  x = evaluate( (prev.tv_sec) + (prev.tv_nsec/BILLION) , poly, sinus, cosinus, expon );

	return (x_1 + x)*( (current.tv_sec) + (current.tv_nsec/BILLION) - (prev.tv_sec) + (prev.tv_nsec/BILLION))/2;
}
double evaluate(double times, struct Poly_t* poly, struct Sin_t* sinus, 
				struct Cos_t* cosinus, struct Exp_t* expon){
	double polRes = 0;
	double sinRes = 0;
	double cosRes = 0;
	double expRes = 0;

	int i = 0;
	//printf("times: %lf\n", times);

	for(i = 0; i < poly->size; i++){
		polRes += (pow(times ,i)*poly->coefs[i]) ;
		// printf("polRes %0.2lf\n", polRes);
	}

	for(i = 0; i < sinus->size; i++){
		sinRes += (pow(times ,i)*sinus->coefs[i]);		
	}	
	if(sinus->size != 0)
		sinRes = sin(sinRes*PI/180);	

	for(i = 0; i < cosinus->size; i++){
		cosRes += (pow(times ,i)*cosinus->coefs[i]);
	}
	if(cosinus->size != 0){
		cosRes = cos(cosRes*PI/180);
	}

	for(i = 0; i < expon->size; i++){
		expRes += (pow(times ,i)*expon->coefs[i]);
	}
	if(expon->size != 0){
		expRes = exp(expRes);
	}

	return polRes + sinRes + cosRes + expRes;
}
void sinOperation(const char* chpExpression, int start, int* coefs){
	//printf("chpExpression %s, index %d\n", chpExpression, index);
	FILE* inp;
	inp = fopen("values.txt", "a");	
	allocation(chpExpression, start, strlen(chpExpression)-2, coefs);	
	fprintf(inp, "s\n");
	fclose(inp);
}
void cosOperation(const char* chpExpression, int start, int* coefs){
	FILE* inp;
	inp = fopen("values.txt", "a");	
	allocation(chpExpression, start, strlen(chpExpression)-2, coefs);
	fprintf(inp, "c\n");
	fclose(inp);
}
void expOperation(const char* chpExpression, int start, int* coefs){
	FILE* inp;
	inp = fopen("values.txt", "a");	
	allocation(chpExpression, start, strlen(chpExpression)-2, coefs);
	fprintf(inp, "e\n");
	fclose(inp);
}

void initialize(const char* chpExpression,int start, int end, int * coefs){
	int index = start;
	int iCoef;
	int iExpo;
	int lastPlus = start-1;
	
	// printf("initialize girdi:\n");
	// printf("chpExpression: %s\n", chpExpression);
	//printf("start %d end: %d\n", start, end);
	
	while(index <= end){
		// printf("index: %d\n", index);
		if(chpExpression[index] == 'x'){
			/* ---*x^---- */
			if(chpExpression[index-1] == '*' && chpExpression[index+1] == '^'){				
				iCoef = findCoef(&chpExpression[0], lastPlus+1, index-1);
				iExpo = isConstant(&chpExpression[index+2]);
				coefs[iExpo] += iCoef;
				//printf("coefs[%d] = %d\n", iExpo, coefs[iExpo]);
				//printf("lastPlus %d\n", lastPlus);
				//printf("index: %d iCoef: %d\n", index, iCoef);
				// printf("coefs[%d]: %d\n", iExpo, coefs[iExpo]);				
			}/* ---*x---- */
			else if(chpExpression[index-1] == '*' && chpExpression[index+1] != '^'){
				iCoef = findCoef(&chpExpression[0], lastPlus+1, index-1);
				coefs[1] += iCoef;
				// printf("coefs[1] %d\n", coefs[1]);

			}/* ---x^--- */
			else if(chpExpression[index-1] != '*' && chpExpression[index+1] == '^'){
				iCoef = 1;
				iExpo = isConstant(&chpExpression[index+2]);
				coefs[iExpo] += 1;
				//printf("coefs[%d] :%d\n", iExpo, coefs[iExpo]);
			}/* ---+x+--- */
			else if(chpExpression[index-1] != '*' && chpExpression[index+1] != '^'){
				iCoef = 1;
				iExpo = 1;
				coefs[1] += 1;
				//printf("coefs[1] %d\n", coefs[1]);
			}
		}else if(chpExpression[index] == '+'){
			if(chpExpression[index+1] == ' ')
				lastPlus = index+1;
			else
				lastPlus = index;
			//printf("lastPlus asdfas %d\n", lastPlus);
		}else if( (chpExpression[index] < 58 && chpExpression[index] > 37)){
			//printf("geldi %c\n", chpExpression[index] );
			if(chpExpression[index-1] != '^'){
				iCoef = isConstant(&chpExpression[index]);
				if(iCoef > 0){
					coefs[0] += iCoef;
					//printf("coefs[0]: %d\n", coefs[0]);
					index += base(iCoef)-1;
				}
					

			}
		}	
		
		++index;
	}
	
}


/* start is index of + operator
   end is previous index of * operator
 */
int findCoef(const char* chpExpression, int start, int end){
	int iCoef = 0;

	//printf("findCoef-- start: %d, end: %d\n", start, end);
	while(start < end){		
		//printf("chpExpression[start]: %c\n", chpExpression[start]);
		if( (chpExpression[start] < 58) && (chpExpression[start] > 47) ){
			iCoef *= 10;
			iCoef += (chpExpression[start]-48); 
			start++;
		}else{
			break;
		}
	}
	//printf("findCoef-- iCoef %d\n", iCoef);

	return iCoef;
}

int findExpo(const char* chpExpression, int start, int* iCur_index){
	int iExpo = 0;
	//printf("findExpo-- start %d, iCur_index %d\n", start, *iCur_index);

	while( (((int)chpExpression[start] > 47) && ((int)chpExpression[start] < 58)) ){
		iExpo *= 10;
		iExpo += (chpExpression[start]-48); 
		start++;
	}
	*iCur_index = start-1;

	//printf("findExpo-- iExpo %d, iCur_index %d\n", iExpo, *iCur_index);
	return iExpo;
}

/* start => 0, end => index of just before 's' or 'c', or 'e' */
int allocation(const char* chpExpression, int start, int end, int* coefs){
	int index = start-1;
	int bigger = 0;
	int expFlag = 1;
	int iExpo;
	int i;
	int finish;
	int lastPlus = 0;
	int error_status = 1;
	int validIndex;
	FILE* inp;
	inp = fopen("values.txt", "a");	

	//printf("start : %d, end: %d\n", start, end);
	if(chpExpression[0] == '^' || chpExpression[0] == '*' || chpExpression[0] == '+'){
		printf("Bir hata var: nearby: %c\n", chpExpression[0]);
		index = end;
		error_status = -1;
	}
	while( (index++) < end){		
		if(chpExpression[index] == ' ')
			continue;
		//printf("index: %d\n", index);
		if(chpExpression[index] == 'x'){
			//printf("flag1\n");
			//printf("next : %c\n", chpExpression[index+1]);
			if(chpExpression[index+1] == '^' && (iExpo = isConstant(&chpExpression[index+2])) > 0 ){							
				//printf("flag2\n");
				if(iExpo > bigger){
					//printf("flag3\n");
					bigger = iExpo;
					expFlag = 0;								
				}
				//printf("iExpo %d bigger %d\n", iExpo, bigger);				
			}
			else if(chpExpression[index+1] == '^' && iExpo <= 0){
				printf("Bir hata var: nearby: %c\n", chpExpression[index+2]);							
				error_status = -1;
				break;
			}else if(chpExpression[index+1] != '+' && chpExpression[index+1] != ')'){
				printf("Bir hata var: nearby: %c\n", chpExpression[index+1]);
				error_status = -1;
				//printf("flag2\n");
				break;
			}else{
				if(bigger == 0){
					bigger = 1;
					expFlag = 0;
				}					
			}			
		}else if(chpExpression[index] == '^'){
			// printf("flag6\n");
			if(chpExpression[index-1] != 'x'){
				// printf("flag7\n");
				printf("Bir hata var: nearby: %c\n", chpExpression[index-1]);
				error_status = -1;
				// printf("flag3\n");
				break;
			}			
		}else if(chpExpression[index] == '*'){
			// printf("flag8\n");
			if( !( (chpExpression[index-1] > 47 && chpExpression[index-1] < 58) && (chpExpression[index+1] == 'x') ) ){
				printf("Bir hata var: before or next of: %c\n", chpExpression[index]);
				error_status = -1;
				break;
			}			
		}else if(chpExpression[index] == '+'){
			lastPlus = index;
			if(chpExpression[index+1] == ' '){
				continue;
			}				
			else if(chpExpression[index-1] == '+' || chpExpression[index+1] == '+' || chpExpression[index+1] == '^' ){
				error_status = -1;
				printf("Bir hata var: nearby: %c\n", chpExpression[index]);							
				break;
			}					
		}

	}
	//printf("expFlag %d\n", expFlag);
	if(expFlag){		
		bigger = 0;
	}
	//printf("bigger %d\n", bigger);
	coefs = (int*)calloc(bigger+1, sizeof(int*));

	for(i = 0; i <= bigger; i++ )
	{
		coefs[i] = 0;
		// printf("coefs[%d] = %d\n", i, coefs[i]);
	}

	//printf("start: %d end %d\n", start ,end);
	//printf("son plus %d\n", lastPlus);
	if(lastPlus != 0){
		end = lastPlus-1;
	}
	/* initialization */
	initialize(&chpExpression[0], start, end, coefs);

	i = 0;

	fprintf(inp, "#%d\n", bigger+1);

	for(i == 0; i <= bigger; i++){
		//printf("coefs[%d]: %d\n", i, coefs[i]);
		fprintf(inp, "%d\n", coefs[i]);
	}

	fclose(inp);

	free(coefs);

	return error_status;
}
int base(int value){
	
	if(value == 0)
		return 0;
	else
		return 1 + base(value/10);
}


int isConstant(const char* chpExpression){
	int res;
	if(chpExpression[0] == '+' || chpExpression[0] == ' ' || chpExpression[0] == '\0' || chpExpression[0] == ')'){
		return 0;
	}else if( chpExpression[0] == '*' || chpExpression[0] == '^' || chpExpression[0] == 'x'){
		return -1;
	}else if(chpExpression[0] > 57 || chpExpression[0] < 48){
		return -1;
	}

	res = isConstant(&chpExpression[1]);
	if(res == 0)
		return ((int)chpExpression[0]-48);
	else if(res == -1)
		return -1;
	else
		return 10*((int)chpExpression[0]-48) + res;
}

void getValues(struct Poly_t* poly, struct Sin_t* sinus, struct Cos_t* cosinus, struct Exp_t* expon){
	FILE* fp;
	int sinStatus = 0;
	int cosStatus = 0;
	int expStatus = 0;
	int swFlag = 1;
	int size;
	int size2;
	char ch;
	char *buffer = NULL;
	size_t nbyte = 10;
	int nread;
	int i;

	fp = fopen("values.txt", "r");
	while((ch = fgetc(fp)) != EOF) {
        if (ch == 's') {
            sinStatus = 1;
        }else if(ch == 'c'){
        	cosStatus = 1;
        }else if(ch == 'e'){
        	expStatus = 1;
        }
    }
    rewind(fp);
    //printf("sin status %d\n", sinStatus);
    ch = fgetc(fp);
    if(ch == '#'){
    	size = (fgetc(fp) - 48);
    }
    // printf("size %d\n", size);
 	poly->coefs = (int*) calloc(size, sizeof(int));
 	poly->size = size;

 	fgetc(fp);
 	// printf("bos: >%c<\n", fgetc(fp)); // \n); 

    for(i = 0; i < size; i++){
		getline(&buffer, &nbyte, fp);
		// printf("getline: %s\n", buffer);
		poly->coefs[i] = atoi(buffer);
    }

    if(sinStatus || cosStatus || expStatus){
	  	ch = fgetc(fp);
	    
	    if(ch == '#'){
	    	size2 = (fgetc(fp) - 48);
	    }
	 	// printf("size2 :%d\n", size2);

	 	if(sinStatus){
	 		sinus->coefs = (int*) calloc(size2, sizeof(int));
	 		sinus->size = size2;
	 		cosinus->size = 0;
	 		expon->size = 0;
	 		fgetc(fp); // \n

		 	for(i = 0; i < size2; i++){
				getline(&buffer, &nbyte, fp);
				//printf("getline: %s\n", buffer);
				sinus->coefs[i] = atoi(buffer);
				//printf("sin %d\n", sinus->coefs[i]);
		    }

	 	}else if (cosStatus){
	 		cosinus->coefs = (int*) calloc(size2, sizeof(int));
	 		sinus->size = 0;
	 		cosinus->size = size2;
	 		expon->size = 0;
	 		fgetc(fp); // \n

		 	for(i = 0; i < size2; i++){
				getline(&buffer, &nbyte, fp);
				//printf("getline: %s\n", buffer);
				cosinus->coefs[i] = atoi(buffer);
				// printf("%d\n", cosinus->coefs[i]);
		    }
	 	}else if (expStatus){
	 		expon->coefs = (int*) calloc(size2, sizeof(int));
	 		sinus->size = 0;
	 		cosinus->size = 0;
	 		expon->size = size2;
	 		fgetc(fp); // \n

		 	for(i = 0; i < size2; i++){
				getline(&buffer, &nbyte, fp);
				//printf("getline: %s\n", buffer);
				expon->coefs[i] = atoi(buffer);
				// printf("%d\n", expon->coefs[i]);
		    }
	 	}    
	}else{
		sinus->size = 0;
		cosinus->size = 0;
		expon->size = 0;
	}
    fclose(fp);
    free(buffer);
    system("rm values.txt");
}

void print_usage(){
	printf("----------------------USAGE---------------------------\n");
	printf("|                                                     |\n");
	printf("|  ./server \"polinomial function\" [sample_time]       |\n");
	printf("|                                                     |\n");
	printf("|  EXAMPLE: ./server \"x^2+3*x+sin(3*x)\" 1000000000    |\n");
	printf("|                                                     |\n");
	printf("------------------------------------------------------\n");
}