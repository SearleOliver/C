#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

void p(int p){
    printf(">> %d reçu par %d \n",p,getpid());
    exit(0);
}
void d(int p){}

int main(int argc, char *argv[]) {   
    if (argc != 3) {
        fprintf(stderr, "Usage: %s NB NBMAX\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int NB = atoi(argv[1]);
    int NBMAX = atoi(argv[2]); 

    struct sigaction act;
    act.sa_handler = d;
    act.sa_flags=0;
    if (sigaction(12,&act,NULL)){
        perror("Echec sigaction");
        exit(EXIT_FAILURE);
    }

    for (int i =0; i< 2;i++ ){
        switch (fork()) {
        case -1:
            perror("Echec fork");
            exit(EXIT_FAILURE);
        case 0:
            for (;;) {
                fscanf(stdin,sizeof(char*NB));
            }
        default:
            wait(NULL);
        }
    }
        
    printf("Il a fini le ptit con.\n");
    return 0;
}
