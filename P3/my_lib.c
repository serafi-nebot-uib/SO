#include "my_lib.h"
#include <sys/fcntl.h>

/*
 * Integrantes del equipo:
 *     Leticia Bargiela de Jesus
 *     Juana Maria Luna Carvajal
 *     Serafí Nebot Ginard
 */

// variable global para obtener el mensaje de error de funciones del sistema
int errno;

/*
 * my_strlen
 * Calcula el número de carácteres dentro de un string terminado en un byte nulo.
 *
 * Argumentos:
 *    const char *str: puntero a la cadena de carácteres
 *
 * Devuelve: entero indicando la cantidad de carácteres dentro del string
 */
size_t my_strlen(const char *str) {
    size_t i = 0;
    for (; str[i]; i++); // iterar la cadena hasta llegar al byte nulo (0 es equivalente a falso)
    return i;
}

/*
 * my_strcmp
 * Compara dos cadenas de carácteres.
 *
 * Argumentos:
 *    const char *str1: puntero a la cadena de carácteres 1
 *    const char *str2: puntero a la cadena de carácteres 2
 *
 * Devuelve: entero indicando si str1 es mayor (>0), menor (<0) o igual (==0) a str2
 */
int my_strcmp(const char *str1, const char *str2) {
    size_t i = 0;
    // iterar ambas cadenas de carácteres hasta llegar al byte nulo de alguna de ellas
    for (; str1[i] && str2[i]; i++)
        if (str1[i] != str2[i]) return str1[i] - str2[i];
    return str1[i] - str2[i];
}

/*
 * my_strcpy
 * Copia una cadena de carácteres a otra.
 *
 * Argumentos:
 *    char *dest: puntero a la cadena de carácteres de destino (dónde se van a copiar los datos)
 *    char *src: puntero a la cadena de carácteres de origen (de dónde se copian los datos)
 *
 * Devuelve: puntero a la cadena de carácteres de destino
 */
char *my_strcpy(char *dest, const char *src) {
    size_t i = 0;
    // iterar la cadena de carácteres de origen hasta llegar al byte nulo i copiar a la cadena de destino, byte a byte
    for (; src[i]; i++) dest[i] = src[i];
    dest[i] = 0; // añadir el byte nulo al final de la cadena para indicar el final
    return dest;
}

/*
 * my_strncpy
 * Copia una cadena de carácteres a otra con un límite de carácteres n.
 * Si n > strlen(src) el resto de carácteres de dest se ponen a 0.
 *
 * Argumentos:
 *    char *dest: puntero a la cadena de carácteres de destino (dónde se van a copiar los datos)
 *    char *src: puntero a la cadena de carácteres de origen (de dónde se copian los datos)
 *    size_t n: número límite de carácteres a copiar
 *
 * Devuelve: puntero a la cadena de carácteres de destino
 */
char *my_strncpy(char *dest, const char *src, size_t n) {
    size_t l = my_strlen(src);
    for (size_t i = 0; i < n; i++)
        dest[i] = i < l ? src[i] : 0; // si hemos superado la longitud de src, ponemos a 0, sino copiamos desde src
    return dest;
}

/*
 * my_strcat
 * Concatena la cadena src al final de la cadena dest.
 *
 * Argumentos:
 *    char *dest: puntero a la cadena de carácteres a la que se va a concatenar
 *    char *src: puntero a la cadena de carácteres que se van a concatenar
 *
 * Devuelve: puntero a la cadena dónde se han concatenado los carácteres
 */
char *my_strcat(char *dest, const char *src) {
    size_t off = my_strlen(dest); // obtener el índice a partir del cual vamos a copiar src
    size_t i = 0;
    for (; src[i]; i++) dest[off+i] = src[i];
    dest[off+i] = 0; // añadir el byte nulo al final de la cadena para indicar el final
    return dest;
}

/*
 * my_strchr
 * Encuentra un carácter dentro de una cadena de carácters.
 *
 * Argumentos:
 *    const char *s: puntero a la cadena de carácteres dónde buscar
 *    int c: carácter que buscar
 *
 * Devuelve: puntero a la primera occurrencia del carácter indicado o NULL en caso de que el carácter no esté dentro de la cadena
 */
char *my_strchr(const char *s, int c) {
    // iterar la cadena hasta llegar al final (byte nulo)
    for (size_t i = 0; s[i]; i++)
        if (s[i] == c) return (char *)(s + i); // si se ha encontrado el carácter indicado, devolver el puntero a dicho carácter
    return NULL;
}

/*
 * my_stack_init
 * Crea un struct my_stack reservando memoria y lo inicializa con el tamaño de datos indicado.
 *
 * Argumentos:
 *    int size: tamaño en bytes de datos para cada nodo
 *
 * Devuelve: puntero al struct my_stack inicializado. NULL en caso de error.
 */
struct my_stack *my_stack_init(int size) {
    struct my_stack *stack = (struct my_stack *)malloc(sizeof(struct my_stack));
    if (stack == NULL) {
        fprintf(stderr, "Error al reservar memoria para la inicialización del stack\n");
        return NULL;
    }
    stack->size = size;
    stack->top = NULL;
    return stack;
}

/*
 * my_stack_push
 * Crea un struct my_node reservando memoria y asignandole los datos indicados.
 * Lo añade a la pila, insertándolo al principio (stack->top apuntara al nuevo nodo).
 *
 * Argumentos:
 *    struct my_stack *: puntero a la pila dónde se va a insertar el nodo
 *    void *data: puntero a los datos del nodo a insertar
 *
 * Devuelve: 0 cuando se ejecuta correctamente, -1 en caso de error
 */
int my_stack_push(struct my_stack *stack, void *data) {
    // si la pila no esta inicializada o tiene un tamaño incorrecto devolvemos error
    if (stack == NULL || stack->size <= 0) return -1;
    struct my_stack_node *node = (struct my_stack_node *)malloc(sizeof(struct my_stack_node));
    if (node == NULL) {
        fprintf(stderr, "Error al reservar memoria para la inicialización del nodo\n");
        return -1;
    }
    node->data = data; // asignar los datos indicados al nuevo nodo
    node->next = stack->top; // el primer nodo se convierte en el segundo
    stack->top = node; // el nuevo nodo se convierte en el primero
    return 0;
}

/*
 * my_stack_pop
 * Saca el pimer nodo de la pila.
 *
 * Argumentos:
 *    struct my_stack *: puntero a la pila de la cuál se va a sacar un nodo
 *
 * Devuelve: puntero a los datos del nodo sacado o NULL en caso de error
 */
void *my_stack_pop(struct my_stack *stack) {
    // si la pila no esta inicializada o tiene un tamaño incorrecto o la pila esta vacía, devolvemos error
    if (stack == NULL || stack->size <= 0 || stack->top == NULL) return NULL;
    struct my_stack_node *node = stack->top;
    stack->top = node->next; // sacar el primer nodo; el segundo nodo ahora es el primero
    // asignar node->data a una variable temporal antes de liberar node para evitar dereferenciar un puntero liberado
    void *data = node->data;
    free(node); // no hace falta comprobar que node != NULL, ya se ha comprobado con el primer if
    return data;
}

/*
 * my_stack_len
 * Calcula la cantidad de nodos en la pila.
 *
 * Argumentos:
 *    struct my_stack *: puntero a la pila de la cuál calcular la cantidad de nodos
 *
 * Devuelve: cantidad de nodos en la pila
 */
int my_stack_len(struct my_stack *stack) {
    // si la pila no esta inicializada o tiene un tamaño incorrecto, asumir que no tiene nodos
    if (stack == NULL || stack->size <= 0) return 0;
    struct my_stack_node *node = stack->top;
    int i = 0;
    // iterar la pila hasta llegar al final (cuando node->next == NULL)
    for (; node != NULL; i++) node = node->next;
    return i;
}

/*
 * my_stack_purge
 * Saca todos los nodos de la pila y libera toda la memoria utilizada.
 *
 * Argumentos:
 *    struct my_stack *: puntero de la pila a liberar
 *
 * Devuelve: cantidad en bytes de memoria liberada
 */
int my_stack_purge(struct my_stack *stack) {
    // si la pila no esta inicializada no hacer nada y devolver 0
    if (stack == NULL) return 0;
    int i = 0;
    for (; stack->top != NULL; i++) {
        if (stack->top->data != NULL) free(stack->top->data); // liberar datos del nodo
        // asignar el puntero del nodo a liberar para evitar dereferenciar un puntero liberado
        void *tmp = stack->top;
        stack->top = stack->top->next; // iterar al siguiente nodo
        free(tmp);
    }
    // calcular el tamaño de datos liberados antes de liberar stack para evitar dereferenciar un puntero liberado
    int size = i * (stack->size + sizeof(struct my_stack_node)) + sizeof(struct my_stack);
    free(stack);
    return size;
}

/*
 * my_stack_read
 * Carga datos de la pila desde un fichero.
 *
 * Argumentos:
 *     char *filename: ruta del fichero del cual se van a cargar los datos
 *
 * Devuelve: puntero a la pila reconstruida a partir de los datos del fichero, NULL en caso de error
 */
struct my_stack *my_stack_read(char *filename) {
    if (filename == NULL) return NULL;

    // abrir fichero indicado con permisos de solo lectura
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Error al abrir el archivo \"%s\": %s\n", filename, strerror(errno));
        return NULL;
    }

    // leer el campo size (primer campo dentro del archivo) que va a determinar el tamaño de datos de cada nodo
    int size = 0;
    ssize_t cnt = read(fd, &size, sizeof(size));
    // comprobar si hay un error de lectura (cnt <= 0) o si el valor leído es incorrecto
    if (cnt <= 0 || size < 0) {
        fprintf(stderr, "Error al leer el archivo \"%s\": %s\n", filename, strerror(errno));
        // cerrar el archivo antes de terminar la ejecución
        if (close(fd) < 0) fprintf(stderr, "Error al cerrar el archivo \"%s\": %s\n", filename, strerror(errno));
        return NULL;
    }

    char success = 1; // indica si el proceso se ha realizado correctamente
    void *buff = malloc(size); // buffer dónde se van a escribir los datos leídos del archivo
    // inicializar stack dónde se van a guardar los datos leídos del archivo
    struct my_stack *stack = my_stack_init(size);
    if (stack != NULL) {
        if (buff != NULL) {
            // crear nodos mientras haya datos a leer en el archivo
            while ((cnt = read(fd, buff, size)) == size && success) {
                void *data = malloc(size); // reservar memoria para los datos del nuevo nodo
                if (data == NULL) {
                    fprintf(stderr, "Error al reservar memoria para los datos del nodo: %s\n", strerror(errno));
                    success = 0;
                } else {
                    memcpy(data, buff, size); // copiar los datos leídos al nodo
                    if (my_stack_push(stack, data) < 0) {
                        // debemos liberar la memoria reservada para los datos del nodo porque my_stack_purge no va a liberarlos (no esta en la pila)
                        free(data);
                        success = 0;
                    }
                }
            }
            // comprobar si read() ha devuelto error
            if (cnt < 0) {
                fprintf(stderr, "Error al leer el archivo \"%s\": %s\n", filename, strerror(errno));
                success = 0;
            }
        } else {
            success = 0;
            fprintf(stderr, "Error al reservar memoria para el buffer de lectura del fichero: %s\n", strerror(errno));
        }
    } else {
        success = 0;
    }

    if (buff != NULL) free(buff); // liberar buffer de lectura del archivo
    if (close(fd) < 0) fprintf(stderr, "Error al cerrar el archivo \"%s\": %s\n", filename, strerror(errno));

    if (success) return stack;

    my_stack_purge(stack); // limpiar la pila en caso de que se haya encontrado un error
    return NULL;
}

/*
 * my_stack_write
 * Escribe los datos de la pila a un archivo.
 *
 * Argumentos:
 *    struct my_stack *: puntero de la pila a escribir
 *    char *filename: ruta del archivo al que escribir
 *
 * Devuelve: la cantidad de nodos escritos o -1 en caso de error
 */
int my_stack_write(struct my_stack *stack, char *filename) {
    if (stack == NULL || filename == NULL) return -1;

    // crear una pila auxiliar para guardar los nodos en orden inverso
    // así cuando se leen con my_stack_read se insertan con el orden original
    struct my_stack *aux = my_stack_init(stack->size);
    if (aux == NULL) return -1;
    struct my_stack_node *node = stack->top;
    while (node != NULL) {
        if (my_stack_push(aux, node->data) < 0) {
            my_stack_purge(aux);
            return -1;
        }
        node = node->next;
    }

    // abrir el archivo i escribir los datos a partir de la pila auxiliar
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        fprintf(stderr, "Error al abrir el archivo \"%s\": %s\n", filename, strerror(errno));
        return -1;
    }
    // escribir el campo size para indicar el tamaño de datos de cada nodo
    if (write(fd, &stack->size, sizeof(int)) < 0) {
        fprintf(stderr, "Error al escribir en el archivo \"%s\": %s\n", filename, strerror(errno));
        return -1;
    }

    int success = 1; // indica si se ha completado el proceso correctamente
    // iterar la pila auxiliar y escribir los datos de cada nodo
    int i = 0;
    for (; aux->top != NULL && success; i++) {
        if (write(fd, aux->top->data, aux->size) < 0) {
            fprintf(stderr, "Error al escribir en el archivo \"%s\": %s\n", filename, strerror(errno));
            success = 0;
        } else {
            // liberar el nodo de la pila auxiliar escrito; el puntero de los datos no se libera
            // asignar puntero al nodo a liberar a una variable temporal para evitar dereferenciar un puntero liberado
            void *tmp = aux->top;
            aux->top = aux->top->next; // iterar al siguiente nodo
            free(tmp);
        }
    }

    // cerrar el archivo y liberar la pila auxiliar antes de terminar la ejecución (liberación de recursos)
    if (close(fd) < 0) fprintf(stderr, "Error al cerrar el archivo \"%s\": %s\n", filename, strerror(errno));
    my_stack_purge(aux);

    if (success) return i;
    return 0;
}
