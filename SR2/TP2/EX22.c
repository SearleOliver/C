#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>

#define MAX_CAPTEURS 10
#define BUFFER_SIZE 256

int tubes[MAX_CAPTEURS][2];
pid_t capteurs[MAX_CAPTEURS];

volatile sig_atomic_t envoyer = 0;

void handler(int sig) {
    envoyer = 1;
}

void capteur(int index, int intervalle, int NBL) {
    signal(SIGUSR1, handler);

    close(tubes[index][0]);  // Ferme lecture côté capteur

    int compteur = 0;
    char c;

    struct itimerval timer;
    timer.it_interval.tv_sec = intervalle;
    timer.it_interval.tv_usec = 0;
    timer.it_value.tv_sec = intervalle;
    timer.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &timer, NULL);
    signal(SIGALRM, handler);

    while (1) {
        if (read(STDIN_FILENO, &c, 1) > 0 && c != '\n') {
            compteur++;
        }

        if (envoyer) {
            char msg[50];
            sprintf(msg, "Capteur %d: %d véhicules\n", index, compteur);
            write(tubes[index][1], msg, strlen(msg));
            envoyer = 0;

            if (compteur >= NBL) {
                pause();  // Attend ordre de terminaison
                break;
            }
        }
    }

    close(tubes[index][1]);
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <NF> <NBL> <intervalle>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int NF = atoi(argv[1]);
    int NBL = atoi(argv[2]);
    int intervalle = atoi(argv[3]);

    for (int i = 0; i < NF; ++i) {
        pipe(tubes[i]);

        pid_t pid = fork();
        if (pid == 0) {
            for (int j = 0; j < NF; ++j) {
                if (j != i) {
                    close(tubes[j][0]);
                    close(tubes[j][1]);
                }
            }
            capteur(i, intervalle, NBL);
        } else {
            capteurs[i] = pid;
            close(tubes[i][1]);  // Ferme écriture côté père
        }
    }

    int terminés = 0;
    char buffer[BUFFER_SIZE];

    while (terminés < NF) {
        for (int i = 0; i < NF; ++i) {
            if (tubes[i][0] == -1) continue;

            int n = read(tubes[i][0], buffer, BUFFER_SIZE - 1);
            if (n > 0) {
                buffer[n] = '\0';
                printf("Panneau reçoit : %s", buffer);

                int valeur;
                sscanf(buffer, "Capteur %*d: %d", &valeur);
                if (valeur >= NBL) {
                    kill(capteurs[i], SIGUSR1);  // Donne l’ordre de terminer
                    close(tubes[i][0]);
                    tubes[i][0] = -1;
                    terminés++;
                }
            }
        }
    }

    for (int i = 0; i < NF; ++i)
        wait(NULL);

    return 0;
}
