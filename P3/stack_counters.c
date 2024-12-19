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

// struct de argumentos para el worker de cada thread
struct thread_opts {
    unsigned long id; // id del thread
    struct my_stack *stack; // puntero al stack
    char *color; // color del thread para diferenciar cada thread de forma visual
};

struct thread_opts thread_args[THREAD_COUNT] = {};
pthread_t threads[THREAD_COUNT] = {};
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// array de colores permitidos para los mensajes de los threads
char *colors[] = { MAG, CYN, GRY, YEL, BLU };
#define COLORS_SIZE (sizeof(colors)/sizeof(*colors))

// THREAD_LOG_EN habilita/deshabilita los mensajes de los threads (util para valores de ITERATION_COUNT muy altos)
#define THREAD_LOG_EN 0
// logging the threads mostrando el id en el color indicado
#define THREAD_LOG(opts, ...) { if (THREAD_LOG_EN) { printf("thread %s%lu " RST, opts.color, opts.id); printf(__VA_ARGS__); printf("\n" RST); }}

void *worker(void *ptr) {
    // no hacer nada si no se han proporcionado argumentos
    if (ptr == NULL) pthread_exit(NULL);
    // obtener los argumentos del thread (struct thread_opts)
    struct thread_opts opts = *(struct thread_opts *)ptr;
    // asignar el id del thread a thread_opts
    opts.id = (unsigned long) pthread_self();
    for (size_t i = 0; i < ITERATION_COUNT; i++) {
        if (pthread_mutex_lock(&mutex) != 0) {
            perror("mutex_lock");
            continue;
        }
        THREAD_LOG(opts, RED "pop");
        int *data = (int *)my_stack_pop(opts.stack);
        if (pthread_mutex_unlock(&mutex) != 0) perror("mutex_unlock");
        if (data == NULL) {
            THREAD_LOG(opts, "no se ha podido obtener el valor del stack");
            pthread_exit(NULL);
        }
        *data += 1;

        if (pthread_mutex_lock(&mutex) != 0) {
            perror("mutex_lock");
            continue;
        }
        THREAD_LOG(opts, GRN "push");
        if (my_stack_push(opts.stack, data) < 0) {
            THREAD_LOG(opts, "no se ha podido poner el valor al stack");
            pthread_exit(NULL);
        }
        if (pthread_mutex_unlock(&mutex) != 0) perror("mutex_unlock");
    }
    pthread_exit(NULL);
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
        fprintf(stderr, RED "error sintaxis; uso: ./stack_counter <nombre_pila>\n" RST);
        return -1;
    }

    printf("threads: %d; iterations: %d\n", THREAD_COUNT, ITERATION_COUNT);

    // si el archivo existe -> leer el stack del archivo
    // sino -> inicializar el stack vacío
    struct my_stack *stack = NULL;
    if (access(argv[1], F_OK) == 0) stack = my_stack_read(argv[1]);
    else stack = my_stack_init(sizeof(int));
    if (stack == NULL) return -1;

    printf("stack->size: %d\n", stack->size);
    printf("initial stack content:\n");
    stack_info(stack);
    printf("initial stack length: %d\n", my_stack_len(stack));

    int len = my_stack_len(stack);
    if (len < DEFAULT_STACK_LEN) {
        for (int i = 0; i < DEFAULT_STACK_LEN - len; i++) {
            // reservar memoria para el nuevo dato
            int *data = (int *)malloc(sizeof(int));
            if (data == NULL) {
                printf("error al reservar memoria para los datos del stack\n");
                my_stack_purge(stack);
                return -1;
            }
            *data = 0;
            if (my_stack_push(stack, data) < 0) {
                my_stack_purge(stack);
                return -1;
            }
        }
        printf("stack content for treatment:\n");
        stack_info(stack);
        printf("new stack length: %d\n", my_stack_len(stack));
    }

    printf("\n");
    for (size_t i = 0; i < THREAD_COUNT; i++) {
         // inicializar argumentos para el nuevo thread (el id se lo va a asignar el propio thread)
        thread_args[i].stack = stack;
        thread_args[i].color = colors[i%COLORS_SIZE];
        pthread_create(&threads[i], NULL, worker, &thread_args[i]);
        printf(RED "[%lu] thread %s%lu" RED " created\n" RST, i, thread_args[i].color, (unsigned long) threads[i]);
    }
    // esperar a que todos los threads terminen la ejecución
    for (size_t i = 0; i < THREAD_COUNT; i++) pthread_join(threads[i], NULL);
    pthread_mutex_destroy(&mutex);

    printf("\n");
    printf("stack content after threads iterations:\n");
    stack_info(stack);
    printf("stack length: %d\n", my_stack_len(stack));

    my_stack_write(stack, argv[1]);
    printf("written elements from stack to file: %d\n", my_stack_len(stack));
    printf("bytes liberados: %d\n", my_stack_purge(stack));
    printf("bye from main\n");
    pthread_exit(NULL);

    return 0;
}
