#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char **argv) {
    if (!argv[1]) {
        fprintf(stderr, "syntax error: sleep <sleep_s>\n");
        return -1;
    }
    int sec = atoi(argv[1]);
    for (int i = 0; i < sec; i++) sleep(1);
    return 0;
}
