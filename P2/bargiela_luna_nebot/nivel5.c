/*
 * Integrantes del equipo:
 *     Leticia Bargiela de Jesus
 *     Juana Maria Luna Carvajal
 *     Serafí Nebot Ginard
 */
#define _POSIX_C_SOURCE 200112L

#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/param.h>

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

#define DEBUGN5 1

#define DEBUG(...) { if (DEBUGN5) { fprintf(stderr, GRY "["); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "]" RST "\n"); }}
#define ERROR(...) { fprintf(stderr, RED); fprintf(stderr, __VA_ARGS__); fprintf(stderr, RST "\n"); }
#define ERRORSYS(s) { fprintf(stderr, RED); perror(s); fprintf(stderr, RST);}

#define min(a, b) (a < b ? a : b)
#define max(a, b) (a > b ? a : b)

#define COMMAND_LINE_SIZE  1024
char cmdbuff[COMMAND_LINE_SIZE] = {};

#define ARGS_SEP "\t\n\r "
#define ARGS_SIZE 64
char *args[ARGS_SIZE] = {};

// buffer global para realizar las llamadas a getcwd()
char cwdbuff[MAXPATHLEN] = {};

int errno;

#define N_JOBS 64

struct info_job {
    pid_t pid;
    char estado; // ‘N’, ’E’, ‘D’, ‘F’ (‘N’: Ninguno, ‘E’: Ejecutándose y ‘D’: Detenido, ‘F’: Finalizado)
    char cmd[COMMAND_LINE_SIZE];
};

size_t n_job = 0;

static struct info_job jobs_list[N_JOBS];
static char mi_shell[COMMAND_LINE_SIZE];

void strrep(char *s, char old, char new);
void print_prompt();
size_t args_count(char **args);
int internal_cd(char **args);
int internal_export(char **args);
int internal_source(char **args);
int internal_jobs(char **args);
int internal_fg(char **args);
int internal_bg(char **args);
int internal_exit(char **args);
char *read_line(char *line);
int check_internal(char **args);
int parse_args(char **args, char *line);
int execute_line(char *line);

void strrep(char *s, char old, char new) {
    for (; *s; s++)
        if (*s == old) *s = new;
}

void print_prompt() {
    char *user = getenv("USER");
    char *cwd = getcwd(cwdbuff, MAXPATHLEN);
    fflush(stdout);
    printf(BCYN "%s" BRED ":" BGRN "%s" BWHT "$ " RST, user, cwd);
    fflush(stdout);
}

size_t args_count(char **args) {
    size_t i = 0;
    while (args[i] != NULL) i++;
    return i;
}

int jobs_list_add(pid_t pid, char estado, char *cmd) {
    // N_JOBS-1 because the first element is reserved for foreground
    if (n_job >= N_JOBS-1) return -1;
    n_job++;
    jobs_list[n_job].pid = pid;
    jobs_list[n_job].estado = estado;
    strncpy(jobs_list[n_job].cmd, cmd, COMMAND_LINE_SIZE);
    DEBUG("jobs_list_add() -> proceso %d (%s) añadido a la lista", pid, cmd);
    return n_job;
}

int jobs_list_find(pid_t pid) {
    for (int i = 0; i <= n_job; i++)
        if (jobs_list[i].pid == pid) return i;
    return -1;
}

int jobs_list_remove(int pos) {
    if (pos >= N_JOBS) return -1;
    struct info_job new = jobs_list[n_job--];
    jobs_list[pos].pid = new.pid;
    jobs_list[pos].estado = new.estado;
    strncpy(jobs_list[pos].cmd, new.cmd, COMMAND_LINE_SIZE);
    return 0;
}

int internal_cd(char **args) {
    size_t argc = args_count(args) - 1; // el primer argumento es el nombre del comando
    char *nwd = NULL; // new working directory
    if (argc == 0) {
        nwd = getenv("HOME");
    } else if (argc == 1) {
        nwd = args[1];
    } else {
    }

    if (nwd == NULL) {
        ERROR("argumentos invalidos");
        return -1;
    }

    if (chdir(nwd) < 0) {
        ERROR("no se ha podido cambiar de directorio: %s", strerror(errno));
        return -1;
    }

    return 0;
}

int internal_export(char **args) {
    char *arg = args[1];
    if (arg == NULL) {
        ERROR("nombre y valor de la variable no especificados");
        return -1;
    }
    char *name = strtok(arg, "=");
    DEBUG("internal_export() -> valor inicial \"%s\" = %s", name, getenv(name));
    char *value = strtok(NULL, "");
    if (value == NULL) {
        ERROR("valor de la variable de entorno no especificado");
        return -1;
    }
    if (setenv(name, value, 1) < 0) {
        ERROR("no se ha podido modificar la variable de entorno \"%s\": %s", name, strerror(errno));
        return -1;
    }
    DEBUG("internal_export() -> valor final \"%s\" = %s", name, getenv(name));
    return 0;
}

int internal_source(char **args) {
    char *path = args[1];
    if (path == NULL) {
        ERROR("sintaxis erronea; uso: source <nombre_fichero>");
        return -1;
    }

    FILE *file = fopen(path, "r");
    if (file == NULL) {
        ERROR("no se ha podido abrir el fichero \"%s\": %s", path, strerror(errno));
        return -1;
    }

    char line[COMMAND_LINE_SIZE] = {};
    memset(line, 0, COMMAND_LINE_SIZE);
    while (fgets(line, COMMAND_LINE_SIZE, file)) {
        strrep(line, '\n', 0);
        fflush(file);
        execute_line(line);
    }

    if (fclose(file) != 0) {
        ERROR("no se ha podido cerrar el fichero \"%s\": %s", path, strerror(errno));
        return -1;
    }

    return 0;
}

void print_job_status(size_t idx) {
    printf("[%lu] %d\t%c\t%s\n", idx, jobs_list[idx].pid, jobs_list[idx].estado, jobs_list[idx].cmd);
}

int internal_jobs(char **args) {
    for (int i = 1; i <= n_job; i++) print_job_status(i);
    return 0;
}

int internal_fg(char **args) {
    DEBUG("internal_fg() -> esta funcion enviará un proceso del background al foreground");
    return 0;
}

int internal_bg(char **args) {
    DEBUG("internal_bg() -> esta funcion reactivara un proceso detenido para que se ejecute en segundo plano");
    return 0;
}

int internal_exit(char **args) {
    printf("\ngoodbye!\n");
    exit(0);
}

char *read_line(char *line) {
    print_prompt();
    char *s = fgets(line, COMMAND_LINE_SIZE, stdin);
    if (s == NULL) {
        if (feof(stdin)) internal_exit(NULL);
        // TODO: comprobar ferror()?
        exit(1); // fgets ha devuelto error
    }
    strrep(s, '\n', 0);
    return s;
}

const char *const internal_cmd[] = { "cd", "export", "source", "jobs", "fg", "bg", "exit" };
int (*internal_fn[])(char **args) = { internal_cd, internal_export, internal_source, internal_jobs, internal_fg, internal_bg, internal_exit };

int check_internal(char **args) {
    if (args[0] == NULL) return 0;
    int size = min(sizeof(internal_cmd) / sizeof(*internal_cmd), sizeof(internal_fn) / sizeof(*internal_fn));
    for (size_t i = 0; i < size; i++)
        if (strcmp(args[0], internal_cmd[i]) == 0)
            return internal_fn[i](args);
    return 1;
}

int parse_args(char **args, char *line) {
    size_t i = 0;
    char *token = strtok(line, ARGS_SEP);
    while (token && i < ARGS_SIZE-1 && token[0] != '#') {
        args[i++] = token;
        token = strtok(NULL, ARGS_SEP);
    }
    args[i] = NULL;
#if DEBUG_LEVEL <= LOG_LEVEL
    for (size_t j = 0; j <= i; j++) DEBUG("parse_args() -> token: %s", args[j]);
#endif
    return 0;
}

static char SIG_STR_EXITED[] = "EXITED";
static char SIG_STR_STOPPED[] = "STOPPED";
static char SIG_STR_SIGNALED[] = "SIGNALED";
static char SIG_STR_CONTINUED[] = "CONTINUED";
static char SIG_STR_UNKOWN[] = "unkown";

char *signal_str(int status) {
    if (WIFEXITED(status)) return SIG_STR_EXITED;
    if (WIFSTOPPED(status)) return SIG_STR_STOPPED;
    if (WIFSIGNALED(status)) return SIG_STR_SIGNALED;
    if (WIFCONTINUED(status)) return SIG_STR_CONTINUED;
    return SIG_STR_UNKOWN;
}

void job_reset(struct info_job *job) {
    job->pid = 0;
    job->estado = 'F';
    memset(job->cmd, 0, COMMAND_LINE_SIZE);
}

void reaper(int signum) {
    printf("\n");
    signal(SIGCHLD, reaper);
    int status = 0;
    int pid = 0;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        DEBUG("reaper() -> recibida señal %s del pid %d", signal_str(status), pid);
        if (pid == jobs_list[0].pid) {
            job_reset(&jobs_list[0]);
        } else {
            int pos = jobs_list_find(pid);
            if (pos < 0) {
                ERROR("no se ha encontrado el pid %d en la lista de procesos", pid);
                continue;
            }
            jobs_list_remove(pos);
            printf("proceso %d finalizado (%s) con el estado: %d\n", jobs_list[0].pid, signal_str(status), status);
            print_prompt();
        }
    }
}

void ctrlc(int signum) {
    printf("\n");
    struct info_job job = jobs_list[0];
    if (job.pid > 0) {
        if (strcmp(job.cmd, mi_shell) != 0) {
            DEBUG("ctrlc() -> señal SIGTERM enviada a %d (%s) por %d (%s)", job.pid, job.cmd, getpid(), mi_shell);
            kill(job.pid, SIGTERM);
        } else {
            DEBUG("ctrlc() -> señal SIGTERM no enviada por %d (%s): el proceso en foreground es el shell", getpid(), mi_shell);
        }
    } else {
        DEBUG("ctrlc() -> señal SIGTERM no enviada por %d (%s): no hay ningun proceso en foreground", getpid(), mi_shell);
    }
    print_prompt();
}

void ctrlz(int signum) {
    printf("\n");
    struct info_job *job = &jobs_list[0];
    if (job->pid > 0) {
        if (strcmp(job->cmd, mi_shell) != 0) {
            DEBUG("ctrlc() -> señal SIGSTOP enviada a %d (%s) por %d (%s)", job->pid, job->cmd, getpid(), mi_shell);
            kill(job->pid, SIGSTOP);
            job->estado = 'D';
            jobs_list_add(job->pid, job->estado, job->cmd);
            job_reset(job);
        } else {
            DEBUG("señal SIGSTOP no enviada por %d (%s): el proceso en foreground es el shell", getpid(), mi_shell);
        }
    } else {
        DEBUG("señal SIGSTOP no enviada por %d (%s): no hay ningun proceso en foreground", getpid(), mi_shell);
    }
    print_prompt();
}

int is_background(char **args) {
    for (int i = 0; i < ARGS_SIZE && args[i] != NULL; i++) {
        if (strcmp(args[i], "&") == 0) {
            args[i] = NULL;
            return 1;
        }
    }
    return 0;
}

int execute_line(char *line) {
    parse_args(args, line);
    if (check_internal(args) > 0) {
        int background = is_background(args);
        pid_t pid = fork();
        if (pid < 0) {
            ERROR("no se ha podido crear un proceso nuevo para \"%s\": %s", args[0], strerror(errno));
            return -1;
        } else if (pid == 0) {
            // child
            signal(SIGINT, SIG_IGN);
            signal(SIGTSTP, SIG_IGN);
            execvp(args[0], args);
            ERROR("%s: no such file or directory", args[0]);
            exit(-1);
        } else {
            // parent
            if (background) {
                size_t job_idx = jobs_list_add(pid, 'E', line);
                if (job_idx > 0) print_job_status(job_idx);
            } else {
                jobs_list[0].pid = pid;
                jobs_list[0].estado = 'E';
                strncpy(jobs_list[0].cmd, line, COMMAND_LINE_SIZE);
                DEBUG("execute_line() -> pid padre: %d (%s)", getpid(), mi_shell);
                DEBUG("execute_line() -> pid hijo: %d (%s)", pid, line);
                pause();
            }
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    jobs_list[0].pid = 0;
    jobs_list[0].estado = 'N';
    memset(jobs_list[0].cmd, 0, COMMAND_LINE_SIZE);

    strncpy(mi_shell, argv[0], COMMAND_LINE_SIZE);

    signal(SIGINT, ctrlc);
    signal(SIGCHLD, reaper);
    signal(SIGTSTP, ctrlz);

    char *line = NULL;
    while ((line = read_line(cmdbuff)) != NULL) execute_line(line);
    return 0;
}
