#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

void p(int p){
    printf(">> %d SIGUSR1 reçu par %d \n",p,getpid());
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s NBS NBF\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    struct sigaction act;
    act.sa_handler = p;
    if (sigaction(SIGUSR1,&act,NULL)){
        perror("Echec sigaction");
        exit(EXIT_FAILURE);
    }

    int NBS = atoi(argv[1]);

    for (;;) {
        sleep(NBS);
        printf("Mon numero est %d, I am a SURGEON!.\n",getpid());
    }
    return 0;
}
