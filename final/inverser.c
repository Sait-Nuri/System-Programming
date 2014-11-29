float determinant(float** a ,float MATRIX_SIZE)
{
  float s=1,det=0,b[MATRIX_SIZE][MATRIX_SIZE];

    int i,j,m,n,c;
    if (MATRIX_SIZE==1)
    {
        return (a[0][0]);
    }
    else
    {
        det=0;
        for (c=0;c<MATRIX_SIZE;c++)
        {
            m=0;
            n=0;
            for (i=0;i<MATRIX_SIZE;i++)
            {
                for (j=0;j<MATRIX_SIZE;j++)
                {
                    b[i][j]=0;
                    if (i != 0 && j != c)
                    {
                        b[m][n]=a[i][j];
                        if (n<(MATRIX_SIZE-2))
                            n++;
                        else
                        {
                            n=0;
                            m++;
                        }
                    }
                }
            }
            det=det + s * (a[0][c] * determinant(b,MATRIX_SIZE-1));
            s=-1 * s;
        }
    }
 
    return (det);
}
 
void cofactor(float num[25][25],float f)
{
    float b[25][25],fac[25][25];
    int p,q,m,n,i,j;
    for (q=0;q<f;q++)
    {
    for (p=0;p<f;p++)
    {
        m=0;
        n=0;
        for (i=0;i<f;i++)
        {
            for (j=0;j<f;j++)
            {
                if (i != q && j != p)
                {
                    b[m][n]=num[i][j];
                    if (n<(f-2))
                        n++;
                    else
                    {
                       n=0;
                       m++;
                    }
                }
            }
        }
        fac[q][p]=pow(-1,q + p) * determinant(b,f-1);
    }
  }
  transpose(num,fac,f);
}
/*Finding transpose of matrix*/ 
void transpose(float num[25][25],float fac[25][25],float r)
{
    int i,j;
    float b[25][25],inverse[25][25],d;

    for (i=0;i<r;i++)
    {
        for (j=0;j<r;j++)
        {
            b[i][j]=fac[j][i];
        }
    }
    d=determinant(num,r);
    for (i=0;i<r;i++)
    {
        for (j=0;j<r;j++)
        {
            inverse[i][j]=b[i][j] / d;
        }
    }
    printf("\n\n\nThe inverse of matrix is : \n");
 
    for (i=0;i<r;i++)
    {
        for (j=0;j<r;j++)
        {
            printf("\t%f",inverse[i][j]);
        }
        printf("\n");
    }
}
