#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

int solde = 0;

typedef struct {
    int id;
    int nb_ops;
    int is_credit;
} thread_data;

void* transaction(void* arg) {
    thread_data *data = (thread_data*)arg;
    srand(pthread_self());

    for (int i = 0; i < data->nb_ops; ++i) {
        int montant = rand() % 100;

        if (data->is_credit) {
            solde += montant;
            printf("Thread %d crédite de %d -> solde = %d\n", data->id, montant, solde);
        } else {
            if (solde < montant)
                printf("Thread %d: solde insuffisant (%d), mais on débite quand même %d\n", data->id, solde, montant);
            solde -= montant;
            printf("Thread %d débite de %d -> solde = %d\n", data->id, montant, solde);
        }

        usleep(100000);  // Temporisation visible
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <NT> <solde_initial> <nb_ops>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int NT = atoi(argv[1]);
    solde = atoi(argv[2]);
    int nb_ops = atoi(argv[3]);

    pthread_t threads[NT];
    thread_data args[NT];

    for (int i = 0; i < NT; ++i) {
        args[i].id = i;
        args[i].nb_ops = nb_ops;
        args[i].is_credit = (i % 2 == 0);  // Alternance crédit/débit
        pthread_create(&threads[i], NULL, transaction, &args[i]);
    }

    for (int i = 0; i < NT; ++i) {
        pthread_join(threads[i], NULL);
    }

    printf("Solde final : %d\n", solde);
    return 0;
}
