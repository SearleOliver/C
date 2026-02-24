#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>


#define MSG "Message du père\n"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <NE>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int NE = atoi(argv[1]);
    int tube[2];

    if (pipe(tube) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {  // Fils
        close(tube[1]);  // Ferme l’écriture

        char buffer[256];
        int n;

        while ((n = read(tube[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[n] = '\0';
            printf("Fils a reçu : %s", buffer);
        }

        close(tube[0]);
        exit(EXIT_SUCCESS);
    } else {  // Père
        close(tube[0]);  // Ferme la lecture

        for (int i = 0; i < NE; ++i) {
            write(tube[1], MSG, strlen(MSG));
        }

        close(tube[1]);  // Ferme l’écriture pour signaler EOF au fils
        wait(NULL);      // Attend la fin du fils
        exit(EXIT_SUCCESS);
    }
}
