#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <pthread.h>
#include <sys/stat.h>

#include "my_lib.h"
#include "colors.h"

#define ITERATION_COUNT 5
#define THREAD_COUNT 3
#define DEFAULT_STACK_LEN THREAD_COUNT

#define min(a,b) (a < b ? a : b)

pthread_t threads[THREAD_COUNT] = {};
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *worker(void *ptr) {
    unsigned long id = (unsigned long) pthread_self();
    if (ptr != NULL) {
        struct my_stack *stack = ptr;
        for (size_t i = 0; i < ITERATION_COUNT; i++) {
            // if (pthread_mutex_lock(&mutex) != 0) {
            //     perror("mutex_lock");
            //     continue;
            // }

            printf(BLU "thread %lu ejecutando pop\n" RST, id);
            int *data = (int *)my_stack_pop(stack);
            if (data == NULL) pthread_exit(NULL); // TODO: print error
            *data += 1;
            printf("thread %lu ejecutando push\n", id);
            if (my_stack_push(stack, data) < 0) {
                printf("%lu failed to push item %lu\n", id, i);
                pthread_exit(NULL); // TODO: print error
            }

            // if (pthread_mutex_unlock(&mutex) != 0) perror("mutex_unlock");
        }
    }
    pthread_exit(NULL);
    return NULL;
}

void stack_info(struct my_stack *stack) {
    struct my_stack_node *ptr = stack->top;
    size_t i = 0;
    while (ptr != NULL) {
        printf("%lu : %d\n", i++, *(int *) ptr->data);
        ptr = ptr->next;
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "error sintaxis; uso: ./stack_counter <nombre_pila>\n");
        return -1;
    }

    printf("threads: %d;iterations: %d\n", THREAD_COUNT, ITERATION_COUNT);

    struct my_stack *stack = NULL;

    if (access(argv[1], F_OK) == 0) stack = my_stack_read(argv[1]);
    else stack = my_stack_init(sizeof(int));
    if (stack == NULL) return -1;

    printf("stack->size: %d\n", stack->size);
    printf("original stack length: %d\n", my_stack_len(stack));

    int len = my_stack_len(stack);
    for (size_t i = 0; i < DEFAULT_STACK_LEN-len; i++) {
        int *data = (int *)malloc(sizeof(int));
        if (data == NULL) return -1; // TODO: print error?
        *data = 0;
        if (my_stack_push(stack, data) < 0) {
            my_stack_purge(stack);
            return -1;
        }
    }

    printf("new stack length: %d\n", my_stack_len(stack));
    stack_info(stack);

    for (size_t i = 0; i < THREAD_COUNT; i++) {
        pthread_create(&threads[i], NULL, worker, stack);
        printf(RED "[%lu] thread %lu created\n" RST, i, (unsigned long) threads[i]);
    }
    for (size_t i = 0; i < THREAD_COUNT; i++) {
        int *result;
        pthread_join(threads[i], (void **)&result);
    }

    stack_info(stack);

    my_stack_write(stack, argv[1]);
    printf("bytes liberados: %d\n", my_stack_purge(stack));
    pthread_exit(NULL);

    return 0;
}
