#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <sys/time.h>
#define SIZE 2000
#define NUM_THREADS 6

long long sequentielle(double* matrice){
    int i , j;
    long long sum=0;
    for(i = 0; i < SIZE; i++){
      for(j = 0; j < SIZE; j++){
        sum+= matrice[i*SIZE + j];
      }
    }
    return sum;
}

long long partielle(double* matrice){
    long long sum = 0;
    long long sump[NUM_THREADS];
    int i;

    for (i = 0; i < NUM_THREADS; i++)
        sump[i] = 0;

    #pragma omp parallel num_threads(NUM_THREADS)
    {
        int id = omp_get_thread_num();
        #pragma omp for
        for (i = 0; i < SIZE; i++)
            for (int j = 0; j < SIZE; j++)
                sump[id] += (long long)matrice[i*SIZE + j];
    }
    for (i = 0; i < NUM_THREADS; i++)
        sum += sump[i];

    return sum;
}

long long partielle2(double* matrice){
    long long sum = 0;
    int i, j;

    #pragma omp parallel num_threads(NUM_THREADS) private(j)
    {
        long long sump = 0;
        #pragma omp for
        for (i = 0; i < SIZE; i++)
            for (j = 0; j < SIZE; j++)
                sump += (long long)matrice[i*SIZE + j];

        #pragma omp atomic
        sum += sump;
    }
    return sum;
}

long long reduction(double* matrice) {
    long long sum = 0;
    int i, j;

    #pragma omp parallel for num_threads(NUM_THREADS) private(j) reduction(+:sum)
    for (i = 0; i < SIZE; i++)
        for (j = 0; j < SIZE; j++)
            sum += (long long)matrice[i*SIZE + j];

    return sum;
}


int main(int argc, char **argv){
    int nb, i , j, k;
    double t,start,stop;
    double* matrice;


    // Allocation
    matrice = (double*) malloc(SIZE*SIZE*sizeof(double)) ;


    // Initialisation
    for (i = 0; i < SIZE; i++)
        for (j = 0; j < SIZE; j++)
            matrice[i*SIZE + j] = 1.0;

    start = omp_get_wtime();
    long long  res = sequentielle(matrice);
    stop = omp_get_wtime();
    t=stop-start;
    printf("Sequentielle : %lld t : %f\n",res,t);

    start = omp_get_wtime();
    long long  res2 = partielle(matrice);
    stop = omp_get_wtime();
    t=stop-start;
    printf("Partielle : %lld t : %f\n",res2,t);

    start = omp_get_wtime();
    long long  res3 = partielle2(matrice);
    stop = omp_get_wtime();
    t=stop-start;
    printf("Partielle2 : %lld t : %f\n",res3,t);

    start = omp_get_wtime();
    long long res4 = reduction(matrice);
    stop = omp_get_wtime();
    t=stop-start;
    printf("Reduction : %lld t : %f\n",res4,t);


    // Liberations
    free(matrice);
    return EXIT_SUCCESS;
}
