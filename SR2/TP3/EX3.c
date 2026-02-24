#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

int *buffer;
int taille, nb_depots, nb_conso;
int in = 0, out = 0;

void* producteur(void* arg) {
    int id = *(int*)arg;
    for (int i = 0; i < nb_depots; ++i) {
        buffer[in] = id * 100 + i;  // Ex: 203 = producteur 2, message 3
        printf("Producteur %d dépose %d à %d\n", id, buffer[in], in);
        in = (in + 1) % taille;
        usleep(100000);
    }
    return NULL;
}

void* consommateur(void* arg) {
    int id = *(int*)arg;
    for (int i = 0; i < nb_conso; ++i) {
        int val = buffer[out];
        printf("Consommateur %d consomme %d à %d\n", id, val, out);
        out = (out + 1) % taille;
        usleep(150000);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        fprintf(stderr, "Usage: %s <NP> <NC> <nb_depots> <nb_conso> <taille>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int NP = atoi(argv[1]), NC = atoi(argv[2]);
    nb_depots = atoi(argv[3]);
    nb_conso = atoi(argv[4]);
    taille = atoi(argv[5]);

    buffer = malloc(sizeof(int) * taille);

    pthread_t prod[NP], cons[NC];
    int ids[NP > NC ? NP : NC];

    for (int i = 0; i < NP; ++i) {
        ids[i] = i;
        pthread_create(&prod[i], NULL, producteur, &ids[i]);
    }

    for (int i = 0; i < NC; ++i) {
        ids[i] = i;
        pthread_create(&cons[i], NULL, consommateur, &ids[i]);
    }

    for (int i = 0; i < NP; ++i) pthread_join(prod[i], NULL);
    for (int i = 0; i < NC; ++i) pthread_join(cons[i], NULL);

    free(buffer);
    return 0;
}
