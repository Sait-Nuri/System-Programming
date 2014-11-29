#include <stdio.h>
#include <stdlib.h>
typedef struct asd
{
	float** db;
}a_t;

int main(int argc, char const *argv[])
{
	float* a;

	a = calloc(10, sizeof(float));

	free(&a[0]);

	return 0;
}

