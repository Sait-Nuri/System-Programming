void* getMatInverse( float** l, float** u, int size){

    int i, j, k, t;
    float a, ratio;
    float invertedM[size][size];

    for( t=0; t<2 ; ++t ){

        if( t == 0 )
            arr2Dcpy( invertedM, l, size );
        else
            arr2Dcpy( invertedM, u, size );        

        for(i = 0; i < size; i++){
            for(j = size; j < 2*size; j++){
                if(i==(j-size))
                    invertedM[i][j] = 1.0;
                else
                    invertedM[i][j] = 0.0;
            }
        }

        for(i = 0; i < size; i++){
            for(j = 0; j < size; j++){
                if(i!=j){
                    ratio = invertedM[j][i]/invertedM[i][i];
                    for(k = 0; k < 2*(size); k++){
                        invertedM[j][k] -= ratio * invertedM[i][k];
                    }
                }
            }
        }

        for(i = 0; i < size; i++){
            a = invertedM[i][i];
            for(j = 0; j < 2*(size); j++){
                invertedM[i][j] /= a;
            }
        }

        k=0;
        for(i = 0; i < size; i++){
            for(j = size; j < 2*(size); j++){
                invertedM[i][k] = invertedM[i][j];
                ++k;
            }
            k=0;
        }


        if( t == 0 )
            arr2Dcpy( l, invertedM , size );
        else
            arr2Dcpy( u, invertedM, size );

    } 
}
void arr2Dcpy( float** mat1, float** mat2, int dim ){

    int i, j;
    for(i = 0; i < dim; ++i) {
        for( j = 0; j < dim; ++j) {
            mat1[i][j] = mat2[i][j];
        }
    }
}