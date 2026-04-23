#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <sys/time.h>
#define SIZE 2000
#define NUM_THREADS 1

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
    int i,j;

    for (i = 0; i < NUM_THREADS; i++)
        sump[i] = 0;

    #pragma omp parallel num_threads(NUM_THREADS)
    {
        int id = omp_get_thread_num();
        #pragma omp for
        for (i = 0; i < SIZE; i++){
            int local = 0;
            for (j = 0; j < SIZE; j++)
                local += (long long)matrice[i*SIZE + j];
            sump[id]+=local;
        }
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
    int i, j;
    double start, stop;
    double* matrice = (double*) malloc(SIZE*SIZE*sizeof(double));

    for (i = 0; i < SIZE; i++)
        for (j = 0; j < SIZE; j++)
            matrice[i*SIZE + j] = 1.0;


    double t_seq[100], t_part[100], t_part2[100], t_red[100];

    for (i = 0; i < 100; i++) {
        start = omp_get_wtime(); sequentielle(matrice); t_seq[i]   = omp_get_wtime() - start;
        start = omp_get_wtime();  partielle(matrice);    t_part[i]  = omp_get_wtime() - start;
        start = omp_get_wtime(); partielle2(matrice);   t_part2[i] = omp_get_wtime() - start;
        start = omp_get_wtime(); reduction(matrice);    t_red[i]   = omp_get_wtime() - start;
    }

    double s1=0, s2=0, s3=0, s4=0;
    for (i = 0; i < 100; i++) { s1+=t_seq[i]; s2+=t_part[i]; s3+=t_part2[i]; s4+=t_red[i]; }
    printf("Sequentielle      moyenne : %f\n", s1/100);
    printf("Partielle         moyenne : %f\n", s2/100);
    printf("Partielle atomique moyenne: %f\n", s3/100);
    printf("Reduction         moyenne : %f\n", s4/100);

    FILE* f = fopen("resultats.csv", "w");
    if (f == NULL) { fprintf(stderr, "Erreur ouverture fichier CSV\n"); return EXIT_FAILURE; }

    fprintf(f, "execution,sequentielle,partielle,partielle_atomique,reduction\n");
    for (i = 0; i < 100; i++)
        fprintf(f, "%d,%.6f,%.6f,%.6f,%.6f\n", i+1, t_seq[i], t_part[i], t_part2[i], t_red[i]);

    fclose(f);
    printf("Résultats exportés dans resultats.csv\n");

    free(matrice);
    return EXIT_SUCCESS;
}
