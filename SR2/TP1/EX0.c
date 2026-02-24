#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s NBS NBF\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int NBF = atoi(argv[2]);
    int NBS = atoi(argv[1]);

    for (int i = 0; i < NBF; i++) {
        sleep(NBS);
        printf("Mon numero est %d, je suis fatigué.\n",getpid());
    }
    return 0;
}
