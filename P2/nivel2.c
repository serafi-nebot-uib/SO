#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// https://gist.github.com/RabaDabaDoba/145049536f815903c79944599c6f952a
// Regular text
#define BLK "\e[0;30m"
#define RED "\e[0;31m"
#define GRN "\e[0;32m"
#define YEL "\e[0;33m"
#define BLU "\e[0;34m"
#define MAG "\e[0;35m"
#define CYN "\e[0;36m"
#define WHT "\e[0;37m"
#define GRY "\e[0;90m"

// Regular bold text
#define BBLK "\e[1;30m"
#define BRED "\e[1;31m"
#define BGRN "\e[1;32m"
#define BYEL "\e[1;33m"
#define BBLU "\e[1;34m"
#define BMAG "\e[1;35m"
#define BCYN "\e[1;36m"
#define BWHT "\e[1;37m"
#define BGRY "\e[1;90m"

// Regular underline text
#define UBLK "\e[4;30m"
#define URED "\e[4;31m"
#define UGRN "\e[4;32m"
#define UYEL "\e[4;33m"
#define UBLU "\e[4;34m"
#define UMAG "\e[4;35m"
#define UCYN "\e[4;36m"
#define UWHT "\e[4;37m"

// Regular background
#define BLKB "\e[40m"
#define REDB "\e[41m"
#define GRNB "\e[42m"
#define YELB "\e[43m"
#define BLUB "\e[44m"
#define MAGB "\e[45m"
#define CYNB "\e[46m"
#define WHTB "\e[47m"

// High intensty background
#define BLKHB "\e[0;100m"
#define REDHB "\e[0;101m"
#define GRNHB "\e[0;102m"
#define YELHB "\e[0;103m"
#define BLUHB "\e[0;104m"
#define MAGHB "\e[0;105m"
#define CYNHB "\e[0;106m"
#define WHTHB "\e[0;107m"

// High intensty text
#define HBLK "\e[0;90m"
#define HRED "\e[0;91m"
#define HGRN "\e[0;92m"
#define HYEL "\e[0;93m"
#define HBLU "\e[0;94m"
#define HMAG "\e[0;95m"
#define HCYN "\e[0;96m"
#define HWHT "\e[0;97m"

// Bold high intensity text
#define BHBLK "\e[1;90m"
#define BHRED "\e[1;91m"
#define BHGRN "\e[1;92m"
#define BHYEL "\e[1;93m"
#define BHBLU "\e[1;94m"
#define BHMAG "\e[1;95m"
#define BHCYN "\e[1;96m"
#define BHWHT "\e[1;97m"

// Reset
#define RST "\e[0m"

#define DEBUG_LEVEL 3
#define INFO_LEVEL 2
#define WARN_LEVEL 1
#define ERROR_LEVEL 0

#define LOG_LEVEL DEBUG_LEVEL

#define DEBUG(...) { if (DEBUG_LEVEL <= LOG_LEVEL) { fprintf(stderr, BGRY "debug: " RST); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); }}
#define INFO(...) { if (INFO_LEVEL <= LOG_LEVEL) { fprintf(stderr, BCYN "info: " RST); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); }}
#define WARN(...) { if (WARN_LEVEL <= LOG_LEVEL) { fprintf(stderr, BYEL "warn: " RST); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); }}
#define ERROR(...) { if (ERROR_LEVEL <= LOG_LEVEL) { fprintf(stderr, BRED "error: " RST); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); }}

#define min(a, b) (a < b ? a : b)
#define max(a, b) (a > b ? a : b)

#define COMMAND_LINE_SIZE  1024
char cmdbuff[COMMAND_LINE_SIZE] = {};

#define ARGS_SEP "\t\n\r "
#define ARGS_SIZE 64
char *args[ARGS_SIZE] = {};

void strrep(char *s, char old, char new) {
    for (; *s; s++)
        if (*s == old) *s = new;
}

void print_prompt() {
    char *user = getenv("USER");
    char *pwd = getenv("PWD");
    fflush(stdout);
    printf(BCYN "%s" BRED ":" BGRN "%s" BWHT "$ " RST, user, pwd);
    fflush(stdout);
}

int internal_cd(char **args) {
    DEBUG("internal_cd()");
    return 0;
}

int internal_export(char **args) {
    DEBUG("internal_export()");
    return 0;
}

int internal_source(char **args) {
    DEBUG("internal_source()");
    return 0;
}

int internal_jobs(char **args) {
    DEBUG("internal_jobs()");
    return 0;
}

int internal_fg(char **args) {
    DEBUG("internal_fg()");
    return 0;
}

int internal_bg(char **args) {
    DEBUG("internal_bg()");
    return 0;
}

int internal_exit(char **args) {
    printf("\ngoodbye!\n");
    exit(0);
}

char *read_line(char *line) {
    print_prompt();
    line = fgets(cmdbuff, COMMAND_LINE_SIZE, stdin);
    if (line == NULL) {
        if (feof(stdin)) internal_exit(NULL);
        // TODO: comprobar ferror()?
        exit(1); // fgets ha devuelto error
    }
    strrep(line, '\n', 0);
    return line;
}

const char *const internal_cmd[] = { "cd", "export", "source", "jobs", "fg", "bg", "exit" };
int (*internal_fn[])(char **args) = { internal_cd, internal_export, internal_source, internal_jobs, internal_fg, internal_bg, internal_exit };

int check_internal(char **args) {
    if (args[0] == NULL) return 0;
    int size = min(sizeof(internal_cmd) / sizeof(*internal_cmd), sizeof(internal_fn) / sizeof(*internal_fn));
    for (size_t i = 0; i < size; i++)
        if (strcmp(args[0], internal_cmd[i]) == 0)
            return internal_fn[i](args);
    return 0;
}

int parse_args(char **args, char *line) {
    // reemplazar '#' por '\0' para evitar que strtok procese tokens posteriores
    strrep(line, '#', 0);
    size_t i = 0;
    for (char *token = strtok(line, ARGS_SEP); token && i < ARGS_SIZE; token = strtok(NULL, ARGS_SEP))
        args[i++] = token;
    args[i] = NULL;
#if DEBUG_LEVEL <= LOG_LEVEL
    for (size_t j = 0; j <= i; j++) DEBUG("parse_args() -> token: %s", args[j]);
#endif
    return 0;
}

int execute_line(char *line) {
    parse_args(args, line);
    check_internal(args);
    return 0;
}

int main(int argc, char **argv) {
    char *line = NULL;
    while ((line = read_line(NULL)) != NULL) execute_line(line);
    return 0;
}
