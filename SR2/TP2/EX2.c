#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define BUFFER_SIZE 256

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <NF> <NBVM> <NBV>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int NF = atoi(argv[1]);
    int NBVM = atoi(argv[2]);
    int NBV = atoi(argv[3]);

    int tubes[NF][2];

    for (int i = 0; i < NF; ++i) {
        if (pipe(tubes[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {  // Fils capteur
            close(tubes[i][0]);  // Ferme lecture

            int count = 0;
            char c;
            while (count < NBVM && read(STDIN_FILENO, &c, 1) > 0) {
                if (c != '\n') {
                    count++;
                    if (count % NBV == 0) {
                        char msg[50];
                        sprintf(msg, "Capteur %d: %d véhicules\n", i, count);
                        write(tubes[i][1], msg, strlen(msg));
                    }
                }
            }

            close(tubes[i][1]);
            exit(EXIT_SUCCESS);
        } else {
            close(tubes[i][1]);  // Ferme écriture côté père
        }
    }

    // Père : panneau d'affichage
    char buffer[BUFFER_SIZE];
    int open_tubes = NF;

    while (open_tubes > 0) {
        for (int i = 0; i < NF; ++i) {
            int n = read(tubes[i][0], buffer, BUFFER_SIZE - 1);
            if (n > 0) {
                buffer[n] = '\0';
                printf("Panneau reçoit : %s", buffer);
            } else if (n == 0) {
                close(tubes[i][0]);
                tubes[i][0] = -1;
                open_tubes--;
            }
        }
    }

    // Attend la fin des fils
    for (int i = 0; i < NF; ++i)
        wait(NULL);

    return 0;
}
