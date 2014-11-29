/* HW03
 * Said Nuri UYANIK
 * 101044067
 * MAT 214
 * Implementation of computing Low and Upper triangular matrix
 */

#include <stdio.h>
#include <math.h>   /* for fabs() */
#include <stdlib.h>
#include <string.h> 
#define ROWS 4
#define COLS 4

int main() {

        
    int i, j, k;
	float temp;
	float up[ROWS][COLS];
	float m[ROWS][COLS] = {
    {10,  2,  3,  4},
    { 6, 17,  8,  9},
    {11, 12, 23, 14},
    {16, 17, 18, 29}
};
	memcpy(up, m, ROWS*COLS*sizeof(float));

	/* Lower triangular matrix */
	printf("Lower triangular matrix\n");
	for(i = 0; i < ROWS; i++)
	{
		for(j = i; j < ROWS ; j++)
		{
			if(i != j)
			{
				temp = m[j][i]/m[i][i];
				for(k = 0; k < ROWS; k++)
				{
					m[j][k] -= ( temp * m[i][k] );
					if(m[j][k] < 0.00000)         
						m[j][k] = fabs(m[j][k]);	/* To get rid of negative zero values like -0.00...*/			
				}
			}
		}	
	}
    
	for(i = 0; i < ROWS; i++)
	{
		for(j = 0; j < COLS; j++)
			printf("%6.1f ", m[i][j]);
		printf("\n");
	}

	/* Upper triangular matrix with lower triangular matrix */
	printf("\n\nUpper triangular matrix \n");
	for(i = ROWS-1; i >= 0; i--)
	{
		for(j = i; j >= 0 ; j--)
		{
			if(i != j && up[i][i] != 0)
			{
				temp = (up[j][i]/up[i][i]);
				for(k = 0; k <= ROWS; k++)
				{
					up[j][k] -= ( temp * up[i][k] );				
					if(up[j][k] < 0.00000)         
						up[j][k] = fabs(up[j][k]);	/* To get rid of negative zero values like -0.00...*/					
				}
			}
		}	
	}

	for(i = 0; i < ROWS; i++)
	{
		for(j = 0; j < COLS; j++)
			printf("%6.1f ", up[i][j]);
		printf("\n");
	}
    return 0;
}


