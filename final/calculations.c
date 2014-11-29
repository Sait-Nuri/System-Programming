#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "matrix.h"

void transposeMatrix(matrix_t **m){
	int i,j;
	matrix_t *transpose;
	transpose = allocateMatrix((*m)->size);

	for(i=0 ; i< (*m)->size ;  ++i)
		for(j=0 ; j< (*m)->size ;  ++j)
			transpose->matrixArr[i][j] = (*m)->matrixArr[j][i];

	freeMatrix(*m);
	*m = transpose;
}

void solve_Lyb(double **L, double *y, double *b, int n){
	int i,j;
	double ll;

	y[0]=b[0]/L[0][0];
 
	for(i=2;i<=n;i++){
		ll=0;
  		for(j=1;j<=i-1;j++){
   			ll=ll+L[i-1][j-1]*y[j-1];
  		}
  		y[i-1]=(b[i-1]-ll)/L[i-1][i-1];
 	}
  
}


void solve_Uxy(double **U, double *x, double *y, int n){
	int i,j;
	double uu;

	x[n-1]=y[n-1]/U[n-1][n-1];
 
	for(i=n-1;i>=1;i--){
  		uu=0;
  		for(j=i+1;j<=n;j++){
   			uu=uu+U[i-1][j-1]*x[j-1];
  		}
  		x[i-1]=(y[i-1]-uu)/U[i-1][i-1];
 	}
}


void lu(double **a,double **l,double **u,int n)
{
	int i=0,j=0,k=0;
	for(i=0;i<n;i++)
	{
		for(j=0;j<n;j++)
		{
			if(j<i)
				l[j][i]=0;
			else
			{
				l[j][i]=a[j][i];
				for(k=0;k<i;k++)
				{
					l[j][i]=l[j][i]-l[j][k]*u[k][i];
				}
			}			
		}
		for(j=0;j<n;j++)
		{
			if(j<i)
				u[i][j]=0;
			else if(j==i)
				u[i][j]=1;
			else
			{
				u[i][j]=a[i][j]/l[i][i];
				for(k=0;k<i;k++)
				{
					u[i][j]=u[i][j]-((l[i][k]*u[k][j])/l[i][i]);	
				}
			}
		}
	}		
	
}

void matrixMultiply(double **factor1,double **factor2,double **product,int size){
 	int i, j, k;


	for(i=0;i<size;i++){
		for(j=0;j<size;j++){
	   		product[i][j]=0;
	    	for(k=0;k<size;k++){
	        	product[i][j]+=factor1[i][k]*factor2[k][j];
	    	}
		}
	}

}

//print matrix to screen
void printMatrix(matrix_t *matrixToPrint){
	int i,j;

	for(i=0 ; i<matrixToPrint->size ; ++i){
		for(j=0 ; j<matrixToPrint->size ; ++j){
			printf("%14f", (matrixToPrint->matrixArr)[i][j]);
		}
		printf("\n");
	}

}

matrix_t *allocateMatrix(int size){
	matrix_t *tempMatrix;
	int i;
	int j;

	tempMatrix = malloc(sizeof(matrix_t));
	tempMatrix->size = size;
	tempMatrix->matrixArr = calloc(size, sizeof(double*));
	for(i=0 ; i< size ; ++i)
		tempMatrix->matrixArr[i] = calloc(size, sizeof(double));

	//Allocate as identity matrix
	for(i=0 ; i < size; ++i)
		for(j=0 ; j < size; ++j)
		{
			if(i == j)
				tempMatrix->matrixArr[i][j]=1;
			else
				tempMatrix->matrixArr[i][j]=0;
		}

	return tempMatrix;
}


void freeMatrix(matrix_t *matrixToFree){
	int i;

	for(i=0 ; i< matrixToFree->size ; ++i)
		free(matrixToFree->matrixArr[i]);
	free(matrixToFree->matrixArr);
	free(matrixToFree);

}

/*Return 1 if m is identity matrix, 0 otherwise*/
int isIdentityMatrix(matrix_t *m){
	int i,j;
	double sum=0;

	for(i=0 ; i < m->size; ++i)
		for(j=0 ; j < m->size; ++j)
		{
			sum += m->matrixArr[i][j];
		}

		if(sum <= (m->size + 0.000001) && sum >= (m->size - 0.000001))
			return 1;
	
	return 0;
}