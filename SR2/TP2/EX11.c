#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>

#define MSG "Message périodique du père\n"

int pipefd[2];
int envois_restants;

void handler(int sig) {
    if (envois_restants > 0) {
        write(pipefd[1], MSG, strlen(MSG));
        envois_restants--;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <intervalle> <nombre_envois>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int intervalle = atoi(argv[1]);
    int NE = atoi(argv[2]);
    envois_restants = NE;

    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {  // Fils
        close(pipefd[1]);

        char buf[256];
        int n;
        while ((n = read(pipefd[0], buf, sizeof(buf)-1)) > 0) {
            buf[n] = '\0';
            printf("Fils a reçu : %s", buf);
        }

        close(pipefd[0]);
        exit(EXIT_SUCCESS);
    } else {  // Père
        close(pipefd[0]);

        struct sigaction sa;
        sa.sa_handler = handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGALRM, &sa, NULL);

        struct itimerval timer;
        timer.it_interval.tv_sec = intervalle;
        timer.it_interval.tv_usec = 0;
        timer.it_value.tv_sec = intervalle;
        timer.it_value.tv_usec = 0;
        setitimer(ITIMER_REAL, &timer, NULL);

        while (envois_restants > 0) {
            // Tâche de fond simulée
            for (volatile long i = 0; i < 100000000; ++i);
            printf("Père continue sa tâche de fond...\n");
            pause();  // Attend le signal
        }

        close(pipefd[1]);
        wait(NULL);
        return 0;
    }
}
