#include <sys/wait.h>
#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
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

#define DEBUGN6 1

#define DEBUG(...) { if (DEBUGN6) { fprintf(stderr, GRY "["); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "]" RST "\n"); }}
#define ERROR(...) { fprintf(stderr, RED); fprintf(stderr, __VA_ARGS__); fprintf(stderr, RST "\n"); }
#define ERRORSYS(s) { perror(RED s RST); }

#define min(a, b) (a < b ? a : b)
#define max(a, b) (a > b ? a : b)

#define COMMAND_LINE_SIZE  1024
char cmdbuff[COMMAND_LINE_SIZE] = {};
char linebuff[COMMAND_LINE_SIZE] = {};

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

int internal_cd(char **args);
int internal_export(char **args);
int internal_source(char **args);
int internal_jobs(char **args);
int internal_fg(char **args);
int internal_bg(char **args);
int internal_exit(char **args);
int execute_line(char *line);

// tabla de comandos internos para evitar encadenar múltiples ifs
const char *const internal_cmd[] = { "cd", "export", "source", "jobs", "fg", "bg", "exit" };
int (*internal_fn[])(char **args) = { internal_cd, internal_export, internal_source, internal_jobs, internal_fg, internal_bg, internal_exit };

static char *SIG_STR_EXITED = "EXITED";
static char *SIG_STR_STOPPED = "STOPPED";
static char *SIG_STR_SIGNALED = "SIGNALED";
static char *SIG_STR_CONTINUED = "CONTINUED";
static char *SIG_STR_UNKOWN = "unkown";

/**
 * Obtener la cadena de texto asociada a un estado de un proceso.
 * @param status: estado del proceso
 * @return cadena de texto asociada al estado del proceso
 */
char *signal_str(int status) {
    if (WIFEXITED(status)) return SIG_STR_EXITED;
    if (WIFSTOPPED(status)) return SIG_STR_STOPPED;
    if (WIFSIGNALED(status)) return SIG_STR_SIGNALED;
    if (WIFCONTINUED(status)) return SIG_STR_CONTINUED;
    return SIG_STR_UNKOWN;
}

/**
 * Resetear los valores de un info_job.
 * @param job: info_job a resetear
 * @return void
 */
void job_reset(struct info_job *job) {
    job->pid = 0;
    job->estado = 'F';
    memset(job->cmd, 0, COMMAND_LINE_SIZE);
}

/**
 * Copiar los datos de un info_job a otro.
 * @param dst: info_job destino
 * @param src: info_job fuente
 */
void job_copy(struct info_job *dst, struct info_job *src) {
    dst->pid = src->pid;
    dst->estado = src->estado;
    memcpy(dst->cmd, src->cmd, COMMAND_LINE_SIZE);
}

/**
 * Reemplazar carácter dentro de un string.
 * @param s: string
 * @param old: carácter a reemplazar
 * @param new: carácter nuevo
 * @return void
 */
void chrrep(char *s, char old, char new) {
    if (s == NULL) return;
    for (; *s; s++)
        if (*s == old) *s = new;
}

/**
 * Imprimir el prompt.
 * @return void
 */
void print_prompt() {
    char *user = getenv("USER");
    char *cwd = getcwd(cwdbuff, MAXPATHLEN);
    fflush(stdout);
    printf(BCYN "%s" BRED ":" BGRN "%s" BWHT "$ " RST, user, cwd);
    fflush(stdout);
}
/**
 * Contar el número de argumentos en un array de strings.
 * @param args: array de strings
 * @return número de argumentos
 */
size_t args_count(char **args) {
    size_t i = 0;
    while (args[i] != NULL) i++;
    return i;
}
/**
 * Añadir un proceso a la lista de procesos.
 * @param pid: pid del proceso
 * @param estado: estado del proceso
 * @param cmd: comando asociado al proceso
 * @return posición del proceso en la lista
 */
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
/**
 * Buscar un proceso en la lista de procesos.
 * @param pid: pid del proceso
 * @return posición del proceso en la lista
 * @return -1 si no se ha encontrado el proceso
 */
int jobs_list_find(pid_t pid) {
    for (int i = 0; i <= n_job; i++)
        if (jobs_list[i].pid == pid) return i;
    return -1;
}
/**
 * Eliminar un proceso de la lista de procesos.
 * @param pos: posición del proceso en la lista
 * @return 0 si se ha eliminado el proceso
 * @return -1 si no se ha eliminado el proceso
 */
int jobs_list_remove(int pos) {
    if (pos >= N_JOBS) return -1;
    struct info_job *src = &jobs_list[n_job--];
    struct info_job *dst = &jobs_list[pos];
    job_copy(dst, src);
    job_reset(src);
    return 0;
}
/**
 * Imprimir el estado de un proceso.
 * @param idx: posición del proceso en la lista
 * @return void
 */
void print_job_status(size_t idx) {
    printf("[%lu] %d\t%c\t%s\n", idx, jobs_list[idx].pid, jobs_list[idx].estado, jobs_list[idx].cmd);
}
/**
 * Cambiar de directorio.
 * @param args: array de strings
 * @return 0 si se ha cambiado de directorio
 * @return -1 si no se ha podido cambiar de directorio
 */
int internal_cd(char **args) {
    size_t argc = args_count(args) - 1; // el primer argumento es el nombre del comando
    char *nwd = NULL; // new working directory
    if (argc == 0) {
        nwd = getenv("HOME");
    } else if (argc == 1) {
        nwd = args[1];
    } else {
        // cd avanzado
    }

    if (nwd == NULL) {
        ERROR("argumentos invalidos");
        return -1;
    }

    if (chdir(nwd) < 0) {
        ERRORSYS("no se ha podido cambiar de directorio");
        return -1;
    }

    return 0;
}
/**
 * Exportar una variable de entorno.
 * @param args: array de strings
 * @return 0 si se ha exportado la variable
 * @return -1 si no se ha podido exportar la variable
 */
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
        perror("no se ha podido modificar la variable de entorno");
        return -1;
    }
    DEBUG("internal_export() -> valor final \"%s\" = %s", name, getenv(name));
    return 0;
}
/**
 * Ejecutar un script.
 * @param args: array de strings
 * @return 0 si se ha ejecutado el script
 * @return -1 si no se ha podido ejecutar el script
 */
int internal_source(char **args) {
    char *path = args[1];
    if (path == NULL) {
        ERROR("sintaxis erronea; uso: source <nombre_fichero>");
        return -1;
    }

    FILE *file = fopen(path, "r");
    if (file == NULL) {
        ERRORSYS("no se ha podido abrir el fichero");
        return -1;
    }

    char line[COMMAND_LINE_SIZE] = {};
    memset(line, 0, COMMAND_LINE_SIZE);
    while (fgets(line, COMMAND_LINE_SIZE, file)) {
        chrrep(line, '\n', 0);
        fflush(file);
        execute_line(line);
    }

    if (fclose(file) != 0) {
        ERRORSYS("no se ha podido cerrar el fichero");
        return -1;
    }

    return 0;
}
/**
 * Listar los procesos en background.
 * @param args: array de strings
 * @return 0 si se han listado los procesos
 */
int internal_jobs(char **args) {
    for (int i = 1; i <= n_job; i++) print_job_status(i);
    return 0;
}
/**
 * Traer un proceso a foreground.
 * @param args: array de strings
 * @return 0 si se ha traido el proceso a foreground
 * @return -1 si no se ha podido traer el proceso a foreground
 */
int internal_fg(char **args) {
    if (args[1] == NULL) {
        ERROR("sintaxis erronea; uso: fg <pos_proceso>");
        return -1;
    }
    int pos = atoi(args[1]);
    if (pos <= 0 || pos > n_job) {
        ERROR("el proceso Nº %d no existe", pos);
        return -1;
    }
    struct info_job *job = &jobs_list[pos];
    if (job->estado == 'D') {
        kill(job->pid, SIGCONT);
        job->estado = 'E';
        DEBUG("internal_fg() -> señal SIGCONT enviada al proceso %d (%s)", job->pid, job->cmd);
    }
    struct info_job *fg_job = &jobs_list[0];

    // remove trailing " &" if it exists
    char *ptrn = " &";
    int n = strlen(ptrn);
    char *end = job->cmd + strlen(job->cmd) - n;
    if (strncmp(end, ptrn, n) == 0)
        while (*end) *end++ = 0;

    job_copy(fg_job, job);
    jobs_list_remove(pos);

    print_job_status(0);
    pause();

    return 0;
}
/**
 * Traer un proceso a background.
 * @param args: array de strings
 * @return 0 si se ha traido el proceso a background
 * @return -1 si no se ha podido traer el proceso a background
 */
int internal_bg(char **args) {
    if (args[1] == NULL) {
        ERROR("sintaxis erronea; uso: bg <pos_proceso>");
        return -1;
    }
    int pos = atoi(args[1]);
    if (pos <= 0 || pos > n_job) {
        ERROR("el proceso Nº %d no existe", pos);
        return -1;
    }
    struct info_job *job = &jobs_list[pos];
    if (job->estado == 'E') {
        ERROR("el proceso %d (%s) ya esta en ejecución", job->pid, job->cmd);
        return -1;
    }
    job->estado = 'E';
    strcat(job->cmd, " &");
    kill(job->pid, SIGCONT);
    DEBUG("internal_fg() -> señal SIGCONT enviada al proceso %d (%s)", job->pid, job->cmd);
    print_job_status(pos);
    return 0;
}
/**
 * Salir del shell.
 * @param args: array de strings
 * @return void
 */
int internal_exit(char **args) {
    printf("\ngoodbye!\n");
    exit(0);
}
/**
 * Reap hijos.
 * @param signum: señal
 * @return void
 */
void reaper(int signum) {
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
            } else {
                jobs_list_remove(pos);
                printf("proceso %d finalizado (%s) con el estado: %d\n", jobs_list[0].pid, signal_str(status), status);
                print_prompt();
            }
        }
    }
}
/**
 * Manejar la señal SIGINT.
 * @param signum: señal
 * @return void
 */
void ctrlc(int signum) {
    printf("\n");
    struct info_job job = jobs_list[0];
    if (job.pid > 0) {
        if (strcmp(job.cmd, mi_shell) != 0) {
            DEBUG("ctrlc() -> señal SIGTERM enviada a %d (%s) por %d (%s)", job.pid, job.cmd, getpid(), mi_shell);
            kill(job.pid, SIGTERM);
        } else {
            DEBUG("ctrlc() -> señal SIGTERM no enviada por %d (%s): el proceso en foreground es el shell", getpid(), mi_shell);
            print_prompt();
        }
    } else {
        DEBUG("ctrlc() -> señal SIGTERM no enviada por %d (%s): no hay ningun proceso en foreground", getpid(), mi_shell);
        print_prompt();
    }
}
/**
 * Manejar la señal SIGTSTP.
 * @param signum: señal
 * @return void
 */
void ctrlz(int signum) {
#if DEBUGN6
    ERROR("\n");
#endif
    struct info_job *job = &jobs_list[0];
    if (job->pid > 0) {
        if (strcmp(job->cmd, mi_shell) != 0) {
            DEBUG("ctrlc() -> señal SIGSTOP enviada a %d (%s) por %d (%s)", job->pid, job->cmd, getpid(), mi_shell);
            kill(job->pid, SIGSTOP);
            job->estado = 'D';
            int pos = jobs_list_add(job->pid, job->estado, job->cmd);
            job_reset(job);
            if (pos > 0) print_job_status(pos);
        } else {
            DEBUG("señal SIGSTOP no enviada por %d (%s): el proceso en foreground es el shell", getpid(), mi_shell);
        }
    } else {
        DEBUG("señal SIGSTOP no enviada por %d (%s): no hay ningun proceso en foreground", getpid(), mi_shell);
    }
}
/**
 * Leer una línea de la entrada estándar.
 * @param line: buffer para almacenar la línea
 * @return puntero a la línea leída
 */
char *read_line(char *line) {
    print_prompt();
    char *s = fgets(line, COMMAND_LINE_SIZE, stdin);
    if (s == NULL) {
        if (feof(stdin)) internal_exit(NULL);
        // TODO: comprobar ferror()?
        exit(1); // fgets ha devuelto error
    }
    chrrep(s, '\n', 0);
    return s;
}
/**
 * Comprobar si el comando es interno.
 * @param args: array de strings
 * @return 0 si el comando es interno
 * @return 1 si el comando no es interno
 */
int check_internal(char **args) {
    if (args[0] == NULL) return 0;
    int size = min(sizeof(internal_cmd) / sizeof(*internal_cmd), sizeof(internal_fn) / sizeof(*internal_fn));
    for (size_t i = 0; i < size; i++)
        if (strcmp(args[0], internal_cmd[i]) == 0)
            return internal_fn[i](args);
    return 1;
}
/**
 * Parsear los argumentos de una línea.
 * @param args: array de strings
 * @param line: línea de entrada
 * @return 0 si todo está bien
 */
int parse_args(char **args, char *line) {
    size_t i = 0;
    char *token = strtok(line, ARGS_SEP);
    while (token && i < ARGS_SIZE-1 && token[0] != '#') {
        args[i++] = token;
        token = strtok(NULL, ARGS_SEP);
    }
    args[i] = NULL;
#if DEBUGN6
    for (size_t j = 0; j <= i; j++) DEBUG("parse_args() -> token: %s", args[j]);
#endif
    return 0;
}
/**
 * Comprobar si el comando se ejecutará en background.
 * @param args: array de strings
 * @return 1 si el comando se ejecutará en background
 * @return 0 si el comando no se ejecutará en background
 */
int is_background(char **args) {
    for (int i = 0; i < ARGS_SIZE && args[i] != NULL; i++) {
        if (strcmp(args[i], "&") == 0) {
            args[i] = NULL;
            return 1;
        }
    }
    return 0;
}
/**
 * Comprobar si se redirige la salida.
 * @param args: array de strings
 * @return 1 si se redirige la salida
 * @return 0 si no se redirige la salida
 */
int is_output_redirection(char **args) {
    for (int i = 0; i < ARGS_SIZE-1 && args[i] != NULL; i++) {
        if (strcmp(args[i], ">") == 0 && args[i+1] != NULL) {
            DEBUG("is_output_redirection() -> redirecting output");
            args[i] = NULL;
            int fd = open(args[i+1], O_CREAT | O_RDWR, 0644);
            if (fd < 0) {
                perror("no se ha podido abrir el archivo");
            } else {
                if (dup2(fd, STDOUT_FILENO) < 0)
                    perror("no se ha podido redireccionar stdout al archivo");
                if (close(fd) < 0)
                    perror("no se ha podido cerrar el archivo");
            }
            return 1;
        }
    }
    return 0;
}
/**
 * Ejecutar una línea de comandos.
 * @param line: línea de entrada
 * @return 0 si todo está bien
 */
int execute_line(char *line) {
    strncpy(linebuff, line, COMMAND_LINE_SIZE);
    parse_args(args, linebuff);
    if (check_internal(args) > 0) {
        int background = is_background(args);
        pid_t pid = fork();
        if (pid < 0) {
            perror("no se ha podido crear un nuevo proceso");
            return -1;
        } else if (pid == 0) {
            // child
            signal(SIGINT, SIG_IGN);
            signal(SIGTSTP, SIG_IGN);
            is_output_redirection(args);
            execvp(args[0], args);
            ERRORSYS("no se ha podido crear el nuevo proceso");
            exit(-1);
        } else {
            // parent
            if (background) {
                int pos = jobs_list_add(pid, 'E', line);
                if (pos > 0) print_job_status(pos);
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

char *arg = "cd \"sistemas operativos\"";

void test() {
    parse_args(args, arg);
    size_t argc = args_count(args) - 1; // el primer argumento es el nombre del comando
    if (argc < 2) return;
}

int main(int argc, char **argv) {
    // test();
    // return 0;

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