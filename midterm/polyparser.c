#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Poly_t
{
	int *coefs;
	int size;
	int sinus;
	int cosinus;
	int exp;
};

struct Sin_t{
	int *coefs;
};

struct Cos_t
{
	int *coefs;	
};

struct Exp_t
{
	int *coefs;	
};

int findCoef(const char*, int, int);
int findExpo(const char*, int, int*);
int allocation(const char*, int , int , int*);
void sinOperation(const char*, int , int*);
void cosOperation(const char*, int , int*);
void expOperation(const char*, int , int*);
int isConstant(const char*);
void initialize(const char*, int, int, int*);

int main(int argc, char const *argv[])
{
	const char* expression = argv[1];
	struct Poly_t poly;
	struct Sin_t sinus;
	struct Cos_t cosinus;
	struct Exp_t expon;
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
	

	exprSize = strlen(expression);
	// printf("size: %d\n", exprSize);

	start = cur_index;

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
			printf("poly coefs[0] %d\n", poly.coefs[0]);
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
	return 0;
}

void sinOperation(const char* expression, int start, int* coefs){
	//printf("expression %s, index %d\n", expression, index);
	FILE* inp;
	inp = fopen("values.txt", "a");	
	allocation(expression, start, strlen(expression)-2, coefs);	
	printf("sin coefs[0] %d\n", coefs[0]);
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
	coefs = (int*)calloc(bigger+1, sizeof(int));

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
		printf("coefs[%d]: %d\n", i, coefs[i]);
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



