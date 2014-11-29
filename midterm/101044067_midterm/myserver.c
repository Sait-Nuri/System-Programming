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

struct clientData
{
	int clientType;
	char clientPid[PID_LENGNT];
	char client_life_str[MAX_TIME];

};

struct sentData
{
	double result;
	long connection_time;
	long elapsed_time;
	int status;
};

struct Poly_t
{
	int *coefs;
	int size;
};

struct Sin_t{
	int *coefs;
	int size;
};

struct Cos_t
{
	int *coefs;	
	int size;
};

struct Exp_t
{
	int *coefs;	
	int size;
};

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

/* flag for signal handling */
volatile sig_atomic_t die_flag = 0; 

/* This static function to handle specified signal */
static void static_func(int signo) {
        
    /* The signal flag */    
    die_flag = 1;
    printf("singal has caught! Please wait....\n");    
}

int main(int argc, char const *argv[])
{
	const char* expression = argv[1];
	struct Poly_t poly;
	struct Sin_t sinus;
	struct Cos_t cosinus;
	struct Exp_t expon;
	struct timespec server_timer;
	struct timespec begin, finish, prev;	
	int serverfd;
	int clientfd;
	int statusfd;
	int cur_index = 0;
	int firstFlag = 1;
	int plusFlag = 1;	
	int allocFlag = 1;
	int controlFlag = 1;
	int coef, expo;
	int indexPlus = 0;
	int index = 0;
	int exprSize;
	int start, end;
	int i = 0;
	long server_send_rate;
	long elapsed;
	long delay;
	double derivRes;
	int time_of_round = 0;
	char client_life[MAX_TIME];
	char client_pid[PID_LENGNT];
	char server_fifo_path[MAX_PATH]; // fifo dosyasının yolu
	char child_fifo_path[MAX_PATH];
	char chpid[6];	
	char valid;
	pid_t pid;
	struct clientData client1;
	struct sentData sentData1;
	FILE* logfile;

	/* Signal handling */
    if (signal(SIGINT, static_func) == SIG_ERR) {
        fputs("An error occurred while setting a signal handler.\n", stderr);
        return EXIT_FAILURE;
   	}
   	
   	exprSize = strlen(expression);
   	start = cur_index;

   	/***************************** --Parsing Part-- *****************************************/
	/* Control mechanism */
	while( (cur_index++) < exprSize){
		//printf("index: %d\n", cur_index);
		/* Sin cos exp göresiye kadar ilerle */
		if(expression[cur_index] == 's' || expression[cur_index] == 'c' || expression[cur_index] == 'e'){			
			/* Allocation for polynome */
			end = cur_index-1;
			
			/* Allocation of polynome */
			if( allocation(expression, start, end, poly.coefs) == -1){
				break;
			}

			/* if not sin, print error, exit operations */
			if( (expression[cur_index+1] == 'i' && expression[cur_index+2] == 'n')) {
				if(expression[cur_index+3] == '('){					
					cur_index += 4; 
					sinOperation(expression, cur_index, sinus.coefs);
					break;
				}else{
					printf("Syntax error at %c\n", expression[cur_index+3]);
					break;
				}									
			}
			else if( (expression[cur_index+1] == 'o' && expression[cur_index+2] == 's')){
				if(expression[cur_index+3] == '('){
					cur_index += 4; 
					cosOperation(expression, cur_index, cosinus.coefs);
					break;
				}else{
					printf("Syntax error at %c\n", expression[cur_index+3]);
				}				
			}
			else if( (expression[cur_index+1] == 'x' && expression[cur_index+2] == 'p')){
				if(expression[cur_index+3] == '('){
					cur_index += 4; 
					expOperation(expression, cur_index, expon.coefs);
					break;
				}else{
					printf("Syntax error at %c\n", expression[cur_index+3]);
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

   	/************************************************************************/

	/******************************* Server part *********************************/

	
	/************************************************************************/
   	printf("Welcome to my server\nMy pid: %d\n", (int)getpid());
   	

   	/* Log file created */
   	if( (logfile = fopen(LOGNAME, "w")) == NULL ){
   		perror("fopen");
   		exit(EXIT_FAILURE);
   	}
   	
   	/* Get server time */
   	clock_gettime(CLOCK_REALTIME,&begin);

	fprintf(logfile, "Holy Math Server borns at %ld microsec.\n", begin.tv_sec*MILLION + begin.tv_nsec/1000);
   	fflush(logfile);
   	/********************* Server timer **********************************/
   	/* rate of sending data to clients */
   	server_send_rate = atol(argv[2]);

   	server_timer.tv_sec = server_send_rate/BILLION;
	server_timer.tv_nsec = server_send_rate - (server_timer.tv_sec*BILLION );
	//printf("server server_timer: %ld %ld\n", server_timer.tv_sec, server_timer.tv_nsec);
	/**********************************************************************/
	

   	/**************** Fifo creating and openning **************/
	sprintf(server_fifo_path, "%s%d", MY_FIFO, getpid());	
	//printf("server_fifo_path: %s\n", server_fifo_path);
	
	/* Create and open fifo */
	if( mkfifo(server_fifo_path, 0666) < 0){
		perror("fifo not created:");
		exit(EXIT_FAILURE);	
	}

	if( (serverfd = open(server_fifo_path, O_RDONLY)) < 0){
		perror("open fifo");
		exit(EXIT_FAILURE);
	}

	/***********************************************************/

		/***************Connect to Client*****************/
	while(1){
		/* Read client's message */
		while(!die_flag){
			if(read(serverfd, (void*)&client1, sizeof(struct clientData) ) > 0)
				break;
			else if(errno == ENOENT){
				perror("read fifo");
				exit(EXIT_FAILURE);
			}
		}		

		if(die_flag){
		// 	fprintf(logfile, "Ctrl+c detected!\n");
		// 	fflush(logfile);
		break;
		}
		/*************************************************/

		/* Get server time */
	   	clock_gettime(CLOCK_REALTIME, &begin);
	   	
	   	/* Initialize the conection time of client */
	   	sentData1.connection_time = (begin.tv_sec*THOUSAND + begin.tv_nsec/MILLION);	
		
		strcpy(client_life ,client1.client_life_str);
		strcpy(client_pid, client1.clientPid);
		
		fprintf(logfile, "Client is connected (%s)\n", client_pid);
		fflush(logfile);

		/* Gelen zaman milisaniye cinsinden olacak */
		// printf("gelen zaman: %ld\n", atol(client_life));
		/* gelen pid */
		//printf("client pid: %s\n", client_pid);

		/* Serverin veri gönderme sıklığı */
		//printf("server rate: %ld\n", server_send_rate);

		/**************** Server Starting time *********************/
		/* Toplamda kaç kere veri gönderilecek */
		time_of_round = (atol(client_life))/(server_send_rate/1000);
		//printf("time_of_round %d\n", time_of_round);
		
		//Zamanı saniye + nanosaniye cinsinden al		
		/***********************************************************/

		/* If message come, create fifo for that client */
		if( 0 > (pid = fork()) ){
			perror("fork");
			exit(EXIT_FAILURE);
		}

		/* Child will send result of math operations */
		if(pid == 0){			
			sprintf(child_fifo_path, "%s%s", MY_FIFO, client_pid); 
			//printf("my child fifo path: %s \n", child_fifo_path);		

			/***************** Create fifo for each client ****************/
			if( mkfifo(child_fifo_path, 0666) < 0){
				perror("fifo not created:");
				exit(EXIT_FAILURE);	
			}	
			//printf("child_fifo oluştu \n");
			/* Client fifo açıldı */
			if( (clientfd = open(child_fifo_path, O_WRONLY)) < 0){
				perror("open fifo");
				exit(EXIT_FAILURE);
			}
			//printf("clientfd açıldı. %d\n", clientfd);
			/*************************************************************/

			/**************Create status fifo for client *****************/
			strcat(child_fifo_path, "status");	
			//printf("status path %s\n", child_fifo_path);

			if( mkfifo(child_fifo_path, 0666) < 0){
				perror("fifo not created:");
				exit(EXIT_FAILURE);	
			}

			if( (statusfd = open(child_fifo_path, O_RDONLY)) < 0){
				perror("open fifo");
				exit(EXIT_FAILURE);
			}
			/*************************************************************/
			//printf("status fifo oluştu\n");

			sentData1.status = 1;
			prev.tv_sec = 0;
			prev.tv_nsec = 0;

			while(time_of_round--)
			{
				if(die_flag){
					printf("asdfasf\n");
					sentData1.status = 0;
					write(clientfd, (void*)&sentData1, sizeof(struct sentData));
					fprintf(logfile, "Ctrl+c detected!\n");					
					fprintf(logfile, "Childs are notified and killed\n");
					fflush(logfile);
					break;
				}
					
				clock_gettime(CLOCK_REALTIME,&finish);								

				/* Stub function of derivative always returns 1 */
				if(client1.clientType == 1)
					sentData1.result = getDeriv(finish, &poly, &sinus, &cosinus, &expon);
				else if(client1.clientType == 2)
					sentData1.result = getInteg(finish, prev, &poly, &sinus, &cosinus, &expon);			

				prev = finish;

				/* Send current micro second */
				sentData1.elapsed_time =  ((finish.tv_sec - begin.tv_sec)*MILLION) + (finish.tv_nsec - begin.tv_nsec)/1000;

				write(clientfd, (void*)&sentData1, sizeof(struct sentData));

				/* Check whether client gets an iterrupt or not*/
				// printf("yazdı\n");
				read(statusfd, &valid, 2);
				//printf("valid: %c\n", valid);

				if(valid == '0'){
					fprintf(logfile,"client %s is died\n", client_pid);
					fflush(logfile);
					break;
				}				
				nanosleep(&server_timer, NULL);
			}

			printf("writing is done\n");
			close(clientfd);
			exit(EXIT_SUCCESS);
		}
		/* Parent don't waits for this client, but listens new clients */
		else{

			
		}
	}

	if(poly.size != 0)
		free(poly.coefs);
	if(sinus.size != 0)
		free(sinus.coefs);
	if(cosinus.size != 0)
		free(cosinus.coefs);
	if(expon.size != 0)
		free(expon.coefs);

	fclose(logfile);
	close(statusfd);
	close(serverfd);	
	unlink(server_fifo_path);
	return 0;
}

double getDeriv(struct timespec curTime, struct Poly_t* poly, struct Sin_t* sinus, 
				struct Cos_t* cosinus, struct Exp_t* expon){
	double x_h;
	double x;

	// Send millisec
	x_h = evaluate( (curTime.tv_sec*THOUSAND) + (curTime.tv_nsec/MILLION) + H , poly, sinus, cosinus, expon);
	  x = evaluate( (curTime.tv_sec*THOUSAND) + (curTime.tv_nsec/MILLION) , poly, sinus, cosinus, expon);

	return (x_h-x)/H;
}
double getInteg(struct timespec current, struct timespec prev,  struct Poly_t* poly, 
				struct Sin_t* sinus, struct Cos_t* cosinus, struct Exp_t* expon){
	double x_1, x;

	x_1 = evaluate( (current.tv_sec*THOUSAND) + (current.tv_nsec/MILLION) , poly, sinus, cosinus, expon );
	  x = evaluate( (prev.tv_sec*THOUSAND) + (prev.tv_nsec/MILLION) , poly, sinus, cosinus, expon );

	return (x_1 + x)*( (current.tv_sec*THOUSAND) + (current.tv_nsec/MILLION) - (prev.tv_sec*THOUSAND) + (prev.tv_nsec/MILLION))/2;
}
double evaluate(double times, struct Poly_t* poly, struct Sin_t* sinus, 
				struct Cos_t* cosinus, struct Exp_t* expon){
	double polRes = 0;
	double sinRes = 0;
	double cosRes = 0;
	double expRes = 0;

	int i = 0;
	//printf("times: %0.2lf\n", times);

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
void sinOperation(const char* expression, int start, int* coefs){
	//printf("expression %s, index %d\n", expression, index);
	FILE* inp;
	inp = fopen("values.txt", "a");	
	allocation(expression, start, strlen(expression)-2, coefs);	
	fprintf(inp, "s\n");
	fclose(inp);
}
void cosOperation(const char* expression, int start, int* coefs){
	FILE* inp;
	inp = fopen("values.txt", "a");	
	allocation(expression, start, strlen(expression)-2, coefs);
	fprintf(inp, "c\n");
	fclose(inp);
}
void expOperation(const char* expression, int start, int* coefs){
	FILE* inp;
	inp = fopen("values.txt", "a");	
	allocation(expression, start, strlen(expression)-2, coefs);
	fprintf(inp, "e\n");
	fclose(inp);
}

void initialize(const char* expression,int start, int end, int * coefs){
	int index = start;
	int coef;
	int expo;
	int lastPlus = start-1;
	
	// printf("initialize girdi:\n");
	// printf("expression: %s\n", expression);
	//printf("start %d end: %d\n", start, end);
	
	while(index <= end){
		// printf("index: %d\n", index);
		if(expression[index] == 'x'){
			/* ---*x^---- */
			if(expression[index-1] == '*' && expression[index+1] == '^'){				
				coef = findCoef(&expression[0], lastPlus+1, index-1);
				expo = isConstant(&expression[index+2]);
				coefs[expo] += coef;
				//printf("coefs[%d] = %d\n", expo, coefs[expo]);
				//printf("lastPlus %d\n", lastPlus);
				//printf("index: %d coef: %d\n", index, coef);
				// printf("coefs[%d]: %d\n", expo, coefs[expo]);				
			}/* ---*x---- */
			else if(expression[index-1] == '*' && expression[index+1] != '^'){
				coef = findCoef(&expression[0], lastPlus+1, index-1);
				coefs[1] += coef;
				// printf("coefs[1] %d\n", coefs[1]);

			}/* ---x^--- */
			else if(expression[index-1] != '*' && expression[index+1] == '^'){
				coef = 1;
				expo = isConstant(&expression[index+2]);
				coefs[expo] += 1;
				//printf("coefs[%d] :%d\n", expo, coefs[expo]);
			}/* ---+x+--- */
			else if(expression[index-1] != '*' && expression[index+1] != '^'){
				coef = 1;
				expo = 1;
				coefs[1] += 1;
				//printf("coefs[1] %d\n", coefs[1]);
			}
		}else if(expression[index] == '+'){
			if(expression[index+1] == ' ')
				lastPlus = index+1;
			else
				lastPlus = index;
			//printf("lastPlus asdfas %d\n", lastPlus);
		}else if( (expression[index] < 58 && expression[index] > 37)){
			//printf("geldi %c\n", expression[index] );
			if(expression[index-1] != '^'){
				coef = isConstant(&expression[index]);
				if(coef > 0){
					coefs[0] += coef;
					//printf("coefs[0]: %d\n", coefs[0]);
					index += base(coef)-1;
				}
					

			}
		}	
		
		++index;
	}
	
}


/* start is index of + operator
   end is previous index of * operator
 */
int findCoef(const char* expression, int start, int end){
	int coef = 0;

	//printf("findCoef-- start: %d, end: %d\n", start, end);
	while(start < end){		
		//printf("expression[start]: %c\n", expression[start]);
		if( (expression[start] < 58) && (expression[start] > 47) ){
			coef *= 10;
			coef += (expression[start]-48); 
			start++;
		}else{
			break;
		}
	}
	//printf("findCoef-- coef %d\n", coef);

	return coef;
}

int findExpo(const char* expression, int start, int* cur_index){
	int expo = 0;
	//printf("findExpo-- start %d, cur_index %d\n", start, *cur_index);

	while( (((int)expression[start] > 47) && ((int)expression < 58)) ){
		expo *= 10;
		expo += (expression[start]-48); 
		start++;
	}
	*cur_index = start-1;

	//printf("findExpo-- expo %d, cur_index %d\n", expo, *cur_index);
	return expo;
}

/* start => 0, end => index of just before 's' or 'c', or 'e' */
int allocation(const char* expression, int start, int end, int* coefs){
	int index = start-1;
	int bigger = 0;
	int expFlag = 1;
	int expo;
	int i;
	int finish;
	int lastPlus = 0;
	int error_status = 1;
	int validIndex;
	FILE* inp;
	inp = fopen("values.txt", "a");

	//printf("start : %d, end: %d\n", start, end);
	if(expression[0] == '^' || expression[0] == '*' || expression[0] == '+'){
		printf("Bir hata var: nearby: %c\n", expression[0]);
		index = end;
		error_status = -1;
	}
	while( (index++) < end){		
		if(expression[index] == ' ')
			continue;
		//printf("index: %d\n", index);
		if(expression[index] == 'x'){
			//printf("flag1\n");
			//printf("next : %c\n", expression[index+1]);
			if(expression[index+1] == '^' && (expo = isConstant(&expression[index+2])) > 0 ){							
				//printf("flag2\n");
				if(expo > bigger){
					//printf("flag3\n");
					bigger = expo;
					expFlag = 0;								
				}
				//printf("expo %d bigger %d\n", expo, bigger);				
			}
			else if(expression[index+1] == '^' && expo <= 0){
				printf("Bir hata var: nearby: %c\n", expression[index+2]);							
				error_status = -1;
				break;
			}else if(expression[index+1] != '+' && expression[index+1] != ')'){
				printf("Bir hata var: nearby: %c\n", expression[index+1]);
				error_status = -1;
				//printf("flag2\n");
				break;
			}else{
				//printf("flag4\n");
				// printf("bigger %d\n", bigger);				
				// printf("expFlag %d\n", expFlag);
				if(bigger == 0){
					//printf("flag5\n");
					bigger = 1;
					expFlag = 0;
					// printf("expFlag %d\n", expFlag);
				}					
			}			
		}else if(expression[index] == '^'){
			// printf("flag6\n");
			if(expression[index-1] != 'x'){
				// printf("flag7\n");
				printf("Bir hata var: nearby: %c\n", expression[index-1]);
				error_status = -1;
				// printf("flag3\n");
				break;
			}			
		}else if(expression[index] == '*'){
			// printf("flag8\n");
			if( !( (expression[index-1] > 47 && expression[index-1] < 58) && (expression[index+1] == 'x') ) ){
				printf("Bir hata var: before or next of: %c\n", expression[index]);
				error_status = -1;
				break;
			}			
		}else if(expression[index] == '+'){
			lastPlus = index;
			if(expression[index+1] == ' '){
				continue;
			}				
			else if(expression[index-1] == '+' || expression[index+1] == '+' || expression[index+1] == '^' ){
				error_status = -1;
				printf("Bir hata var: nearby: %c\n", expression[index]);							
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
	initialize(&expression[0], start, end, coefs);

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


int isConstant(const char* expression){
	int res;
	if(expression[0] == '+' || expression[0] == ' ' || expression[0] == '\0' || expression[0] == ')'){
		return 0;
	}else if( expression[0] == '*' || expression[0] == '^' || expression[0] == 'x'){
		return -1;
	}else if(expression[0] > 57 || expression[0] < 48){
		return -1;
	}

	res = isConstant(&expression[1]);
	if(res == 0)
		return ((int)expression[0]-48);
	else if(res == -1)
		return -1;
	else
		return 10*((int)expression[0]-48) + res;
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
    // printf("sin status %d\n", sinStatus);
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

