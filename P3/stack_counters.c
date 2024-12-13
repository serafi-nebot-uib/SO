#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <pthread.h>
#include <sys/stat.h>

#include "my_lib.h"
#include "colors.h"

#define ITERATION_COUNT 1000000
#define THREAD_COUNT 10
#define DEFAULT_STACK_LEN THREAD_COUNT

#define min(a,b) (a < b ? a : b)

struct thread_opts {
    unsigned long id;
    struct my_stack *stack;
    char *color;
};

struct thread_opts thread_args[THREAD_COUNT] = {};
pthread_t threads[THREAD_COUNT] = {};
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

char *colors[] = { MAG, CYN, GRY, YEL, BLU };
#define COLORS_SIZE (sizeof(colors)/sizeof(*colors))

#define THREAD_LOG_EN 0
#define THREAD_LOG(opts, ...) { if (THREAD_LOG_EN) { printf("thread %s%lu " RST, opts.color, opts.id); printf(__VA_ARGS__); printf("\n" RST); }}

void *worker(void *ptr) {
    if (ptr != NULL) {
        struct thread_opts opts = *(struct thread_opts *)ptr;
        opts.id = (unsigned long) pthread_self();
        for (size_t i = 0; i < ITERATION_COUNT; i++) {
            if (pthread_mutex_lock(&mutex) != 0) {
                perror("mutex_lock");
                continue;
            }
            THREAD_LOG(opts, RED "pop");
            int *data = (int *)my_stack_pop(opts.stack);
            if (pthread_mutex_unlock(&mutex) != 0) perror("mutex_unlock");
            if (data == NULL) pthread_exit(NULL); // TODO: print error
            *data += 1;

            if (pthread_mutex_lock(&mutex) != 0) {
                perror("mutex_lock");
                continue;
            }
            THREAD_LOG(opts, GRN "push");
            if (my_stack_push(opts.stack, data) < 0) {
                THREAD_LOG(opts, "failed to push item: %lu", i);
                pthread_exit(NULL);
            }
            if (pthread_mutex_unlock(&mutex) != 0) perror("mutex_unlock");
        }
    }
    pthread_exit(NULL);
    return NULL;
}

void stack_info(struct my_stack *stack) {
    struct my_stack_node *ptr = stack->top;
    while (ptr != NULL) {
        printf("%d\n", *(int *) ptr->data);
        ptr = ptr->next;
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "error sintaxis; uso: ./stack_counter <nombre_pila>\n");
        return -1;
    }

    printf("threads: %d; iterations: %d\n", THREAD_COUNT, ITERATION_COUNT);

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
    printf("original stack content:\n");
    stack_info(stack);

    for (size_t i = 0; i < THREAD_COUNT; i++) {
        thread_args[i].stack = stack;
        thread_args[i].color = colors[i%COLORS_SIZE];
        pthread_create(&threads[i], NULL, worker, &thread_args[i]);
        printf(RED "[%lu] thread %s%lu" RED " created\n" RST, i, thread_args[i].color, (unsigned long) threads[i]);
    }
    for (size_t i = 0; i < THREAD_COUNT; i++) {
        int *result;
        pthread_join(threads[i], (void **)&result);
    }

    printf("stack content after threads iterations:\n");
    stack_info(stack);

    my_stack_write(stack, argv[1]);
    printf("bytes liberados: %d\n", my_stack_purge(stack));
    pthread_exit(NULL);

    return 0;
}
