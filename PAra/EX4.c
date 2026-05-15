#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <sys/time.h>
#define SIZE 2000
#define NUM_THREADS 12
#define MAX_NUM_OBJ 100

int num_obj = 0;
int capacity;
int weight[MAX_NUM_OBJ];
int utility[MAX_NUM_OBJ];

void read_problem(char *filename){
  char line[256];

  FILE *problem = fopen(filename,"r");
  if (problem == NULL){
    fprintf(stderr,"File %s not found.\n",filename);
    exit(-1);
  }

  while (fgets(line, 256, problem) != NULL){
    switch(line[0]){
        case 'c': // capacity
            if (sscanf(&(line[2]),"%d\n", &capacity) != 1){
                fprintf(stderr,"Error in file format in line:\n");
                fprintf(stderr, "%s", line);
                exit(-1);
            }
            break;

        case 'o': // graph size
            if (num_obj >= MAX_NUM_OBJ){
                fprintf(stderr,"Too many objects (%d): limit is %d\n", num_obj, MAX_NUM_OBJ);
                exit(-1);
            }
            if (sscanf(&(line[2]),"%d %d\n", &(weight[num_obj]), &(utility[num_obj])) != 2){
                fprintf(stderr,"Error in file format in line:\n");
                fprintf(stderr, "%s", line);
                exit(-1);
            }
            else
                num_obj++;
            break;

        default:
            break;
    }
  }
  if (num_obj == 0){
    fprintf(stderr,"Could not find any object in the problem file. Exiting.");
    exit(-1);
  }

}


void sequentiel(int* M, int* U, int S[][capacity+1]){
    for (int j=0; j<=capacity;j++){
        if (M[0]<=j)
            S[0][j]=U[0];
        else
            S[0][j]=0;
    }
    
    for (int i =1; i < num_obj;i++){
        for (int j =0; j <= capacity; j++){
            if (M[i]<=j && S[i-1][j-M[i]]+U[i]>S[i-1][j])
                S[i][j]=S[i-1][j-M[i]]+U[i];
            else 
                S[i][j]=S[i-1][j];
        }   
    }
}

void parallel(int* M, int* U, int S[][capacity+1],int threads){
    for (int j=0; j<=capacity;j++){
        if (M[0]<=j)
            S[0][j]=U[0];
        else
            S[0][j]=0;
    }
    #pragma omp parallel num_threads(threads)
    {
    for (int i = 1; i < num_obj; i++){
        #pragma omp for
        for (int j = 0; j <= capacity; j++){
            if (M[i]<=j && S[i-1][j-M[i]]+U[i] > S[i-1][j])
                S[i][j] = S[i-1][j-M[i]] + U[i];
            else
                S[i][j] = S[i-1][j];
        }
    }
    }
}


int main(int argc, char **argv){
    int i, j;
    double start, stop;

    read_problem("pb1.txt");
    int (*S1)[capacity+1] = malloc(sizeof(int[capacity+1]) * num_obj);
    sequentiel(weight,utility,S1);
    printf("Resultat Problème 1\n");
    for (int i =0; i<num_obj;i++){
        for (int j =0; j<=capacity;j++){
            printf("%d",S1[i][j]);
        }
        printf("\n");
    }
    printf("Meilleure Utilitée : %d\n",S1[num_obj-1][capacity]);
    num_obj=0;
    read_problem("pb2.txt");
    int (*S2)[capacity+1] = malloc(sizeof(int[capacity+1]) * num_obj);
    sequentiel(weight,utility,S2);
    printf("Resultat Problème 2\n");
    printf("Meilleure Utilitée : %d\n",S2[num_obj-1][capacity]);
    num_obj=0;
    read_problem("pb3.txt");
    int (*S3)[capacity+1] = malloc(sizeof(int[capacity+1]) * num_obj);
    sequentiel(weight,utility,S3);
    printf("Resultat Problème 3\n");
    printf("Meilleure Utilitée : %d\n",S3[num_obj-1][capacity]);

    double t_seq[100], t_para2[100], t_para4[100], t_para8[100],t_para16[100];

    for (int i = 0; i < 100; i++) {
        start = omp_get_wtime(); sequentiel(weight,utility,S3); t_seq[i]   = omp_get_wtime() - start;
        start = omp_get_wtime(); parallel(weight,utility,S3,2);    t_para2[i]  = omp_get_wtime() - start;
        start = omp_get_wtime(); parallel(weight,utility,S3,4);   t_para4[i] = omp_get_wtime() - start;
        start = omp_get_wtime(); parallel(weight,utility,S3,8);     t_para8[i]   = omp_get_wtime() - start;
        start = omp_get_wtime(); parallel(weight,utility,S3,16);     t_para16[i]   = omp_get_wtime() - start;
    }

    double s1=0, s2=0, s3=0, s4=0, s5=0;
    for (i = 0; i < 100; i++) { s1+=t_seq[i]; s2+=t_para2[i]; s3+=t_para4[i]; s4+=t_para8[i]; s5+=t_para16[i]; }
    printf("Sequentielle      moyenne : %f\n", s1/100);
    printf("Parallel 2        moyenne : %f\n", s2/100);
    printf("Parallel 4        moyenne: %f\n", s3/100);
    printf("Parallel 8        moyenne : %f\n", s4/100);
    printf("Parallel 16       moyenne : %f\n", s5/100);

    FILE* f = fopen("resultats.csv", "w");
    if (f == NULL) { fprintf(stderr, "Erreur ouverture fichier CSV\n"); return EXIT_FAILURE; }

    fprintf(f, "execution, sequentielle,para2,para4,para8,para16\n");
    for (i = 0; i < 100; i++)
        fprintf(f, "%d,%.6f,%.6f,%.6f,%.6f,%.6f\n", i+1, t_seq[i], t_para2[i], t_para4[i], t_para8[i],t_para16[i]);

    fclose(f);
    printf("Résultats exportés dans resultats.csv\n");

    
    free(S1);
    free(S2);
    free(S3);
    return EXIT_SUCCESS;
}
