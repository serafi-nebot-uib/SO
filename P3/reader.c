#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <pthread.h>
#include <limits.h>
#include <sys/stat.h>

#include "my_lib.h"
#include "colors.h"

#define min(a,b) (a < b ? a : b)
#define max(a,b) (a > b ? a : b)

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, RED "error sintaxis; uso: ./reader <nombre_pila>\n" RST);
        return -1;
    }

    struct my_stack *stack = my_stack_read(argv[1]);
    if (stack == NULL) {
        fprintf(stderr, RED "couldn't open stack file \"%s\"\n" RST, argv[1]);
        return 0;
    }

    struct my_stack_node *ptr = stack->top;
    int len = my_stack_len(stack);
    printf("stack length: %d\n", len);

    int min_val = INT_MAX, max_val = 0;
    long sum = 0;
    while (ptr != NULL) {
        int value = *(int *)ptr->data;
        printf("%d\n", value);
        sum += value;
        min_val = min(min_val, value);
        max_val = max(max_val, value);
        ptr = ptr->next;
    }

    int avg = sum / len;
    printf("\nitems: %d; sum: %ld; min: %d; max: %d; average: %d\n", len, sum, min_val, max_val, avg);
    my_stack_purge(stack);

    return 0;
}
