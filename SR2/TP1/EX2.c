#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

void p(int p){
    printf(">> %d SIGINT reçu par %d \n",p,getpid());
    exit(0);
}
void d(int p){}

int main(int argc, char *argv[]) {    
    struct sigaction act;
    act.sa_handler = d;
    act.sa_flags=0;
    if (sigaction(SIGINT,&act,NULL)){
        perror("Echec sigaction");
        exit(EXIT_FAILURE);
    }


    switch (fork()) {
    case -1:
        perror("Echec fork");
        exit(EXIT_FAILURE);
    case 0:
        act.sa_handler = p;
        act.sa_flags=0;
        if (sigaction(SIGINT,&act,NULL)){
            perror("Echec sigaction");
            exit(EXIT_FAILURE);
        }
        for (;;) {
            sleep(1);
            printf("Mon numero est %d, I AM A SURGEON!\n",getpid());
        }
    default:
        wait(NULL);
    }
    printf("Il a fini le ptit con.\n");
    return 0;
}
