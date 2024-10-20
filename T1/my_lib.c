#include "my_lib.h"
#include <sys/fcntl.h>

int errno;

size_t my_strlen(const char *str) {
	size_t i = 0;
	for (; str[i]; i++);
	return i;
}

int my_strcmp(const char *str1, const char *str2) {
	size_t i = 0;
	for (; str1[i] && str2[i]; i++)
		if (str1[i] != str2[i]) return str1[i] - str2[i];
	return str1[i] - str2[i];
}

char *my_strcpy(char *dest, const char *src) {
	size_t i = 0;
	for (; src[i]; i++) dest[i] = src[i];
	dest[i] = 0;
	return dest;
}

char *my_strncpy(char *dest, const char *src, size_t n) {
	size_t l = my_strlen(src);
	for (size_t i = 0; i < n; i++) dest[i] = i < l ? src[i] : 0;
	return dest;
}

char *my_strcat(char *dest, const char *src) {
	size_t off = my_strlen(dest);
	for (size_t i = 0; src[i]; i++) dest[off+i] = src[i];
	return dest;
}

char *my_strchr(const char *s, int c) {
	for (size_t i = 0; s[i]; i++)
		if (s[i] == c) return (char *)(s + i);
	return NULL;
}

struct my_stack *my_stack_init(int size) {
	struct my_stack *stack = (struct my_stack *)malloc(size);
	if (stack == NULL) {
		fprintf(stderr, "Error al reservar memoria para la inicialización del stack\n");
		return NULL;
	}
	stack->size = size;
	stack->top = NULL;
	return stack;
}

int my_stack_push(struct my_stack *stack, void *data) {
	if (stack == NULL || stack->size <= 0) return -1;
	struct my_stack_node *node = (struct my_stack_node *)malloc(sizeof(struct my_stack_node));
	if (node == NULL) {
		fprintf(stderr, "Error al reservar memoria para la inicialización del nodo\n");
		return -1;
	}
	node->data = data;
	node->next = stack->top;
	stack->top = node;
	return 0;
}

void *my_stack_pop(struct my_stack *stack) {
	if (stack == NULL || stack->size <= 0 || stack->top == NULL) return NULL;
	struct my_stack_node *node = stack->top;
	stack->top = node->next;
	void *data = node->data;
	free(node);
	return data;
}

int my_stack_len(struct my_stack *stack) {
	if (stack == NULL || stack->size <= 0) return 0;
	struct my_stack_node *node = stack->top;
	int i = 0;
	for (; node != NULL; i++) node = node->next;
	return i;
}

int my_stack_purge(struct my_stack *stack) {
	if (stack == NULL || stack->size <= 0) return 0;
	int i = 0;
	for (; stack->top != NULL; i++) {
		if (stack->top->data != NULL) free(stack->top->data);
		void *tmp = stack->top;
		stack->top = stack->top->next;
		free(tmp);
	}
	// calculate size before freeing the stack pointer
	int size = i * (stack->size + sizeof(struct my_stack_node)) + sizeof(struct my_stack);
	free(stack);
	return size;
}

struct my_stack *my_stack_read(char *filename) {
	if (filename == NULL) return NULL;

	int fd = open(filename, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Error al abrir el archivo \"%s\": %s\n", filename, strerror(errno));
		return NULL;
	}

	int size = 0;
	ssize_t r = read(fd, &size, sizeof(size));
	if (r <= 0 || size < 0) {
		fprintf(stderr, "Error al leer el archivo \"%s\": %s\n", filename, strerror(errno));
		if (close(fd) < 0) fprintf(stderr, "Error al cerrar el archivo \"%s\": %s\n", filename, strerror(errno));
		return NULL;
	}

	struct my_stack *stack = my_stack_init(size);
	if (stack == NULL) return NULL;
	void *buff = malloc(size);
	if (buff == NULL) {
		fprintf(stderr, "Error al reservar memoria para los datos del nodo: %s\n", strerror(errno));
		return NULL;
	}

	char success = 1;
	while ((r = read(fd, buff, size)) == size) {
		void *data = malloc(size);
		if (data == NULL) {
			fprintf(stderr, "Error al reservar memoria para los datos del nodo: %s\n", strerror(errno));
			success = 0;
			break;
		}
		memcpy(data, buff, size);
		if (my_stack_push(stack, data) < 0) {
			success = 0;
			break;
		}
	}

	if (r < 0) {
		fprintf(stderr, "Error al leer el archivo \"%s\": %s\n", filename, strerror(errno));
		success = 0;
	}

	free(buff);
	if (close(fd) < 0) fprintf(stderr, "Error al cerrar el archivo \"%s\": %s\n", filename, strerror(errno));

	if (success) return stack;

	my_stack_purge(stack);
	return NULL;
}

int my_stack_write(struct my_stack *stack, char *filename) {
	if (stack == NULL || filename == NULL) return -1;

	// create an auxiliary stack to store nodes in reverse order
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

	// open specified file and write data from the auxiliary stack
	int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (fd < 0) return -1;
	if (write(fd, &stack->size, sizeof(int)) < 0) return -1;

	int i = 0;
	for (; aux->top != NULL; i++) {
		// write fail check
		write(fd, aux->top->data, aux->size);
		void *tmp = aux->top;
		aux->top = aux->top->next;
		free(tmp);
	}

	if (close(fd) < 0) fprintf(stderr, "Error al cerrar el archivo \"%s\": %s\n", filename, strerror(errno));
	return i;
}
