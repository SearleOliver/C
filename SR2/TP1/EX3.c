#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

void p(int p){
    printf(">> %d SIGINT reçu par %d \n",p,getpid());
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s NBS NBF\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    struct sigaction act;
    act.sa_handler = p;
    act.sa_flags=0;
    sigemptyset(&act.sa_mask);
    if (sigaction(SIGINT,&act,NULL)){
        perror("Echec sigaction");
        exit(EXIT_FAILURE);
    }
    printf("Je m'appelle : %d, et je suis protéger de SIGINT.\n",getpid());
    fflush(stdout);
    execlp("/home/ollie_towv/C/SR2/TP1/ex0","/home/ollie_towv/C/SR2/TP1/ex0",argv[1],argv[2],NULL);
    perror("Erreur execlp");
    return 0;
}