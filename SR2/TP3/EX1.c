#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

typedef struct {
    int id;
    int NF;
} thread_arg;

void* thread_func(void* arg) {
    thread_arg *data = (thread_arg*)arg;
    srand(pthread_self());
    for (int i = 0; i < data->NF; ++i) {
        printf("Thread %d affiche %d\n", data->id, i + 1);
        usleep(rand() % 100000);  // 0 à 100ms
    }
    return (void*)(long)data->id;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <NA> <NF>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int NA = atoi(argv[1]);
    int NF = atoi(argv[2]);

    pthread_t threads[NA];
    thread_arg args[NA];

    for (int i = 0; i < NA; ++i) {
        args[i].id = i;
        args[i].NF = NF;
        pthread_create(&threads[i], NULL, thread_func, &args[i]);
    }

    for (int i = 0; i < NA; ++i) {
        void *res;
        pthread_join(threads[i], &res);
        printf("Thread de rang %ld terminé\n", (long)res);
    }

    return 0;
}
