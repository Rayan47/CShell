#include <asm-generic/errno-base.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

char **parse_command(char *line, int *argc);

int execute_command(char *const argv[], int timeout_seconds);

int execute_internal(char *const argv[], int *argc);

void free_command(char **argv);

void print_child_status(pid_t pid, int status);

int main(void) {

    printf("Shell Initialized <3\n");

    char *line = NULL;
    size_t size = 0; // bytes
    ssize_t n_chars = 0;
    int argc;
    char **argv = NULL;

    do {
        printf("shell@%s>", getcwd(NULL, 0));
        fflush(stdout);

        n_chars = getline(&line, &size, stdin);

        if (feof(stdin)) {
            printf("Goodbye!\n");
            free(line);
            free_command(argv);
            return 0;
        } else if (ferror(stdin)) {
            perror("stdin");
            free(line);
            exit(EXIT_FAILURE);
        }

        argv = parse_command(line, &argc);
        if (argv == NULL) {
            perror("Failed to parse command");
            continue;
        }
        if (argc == 0) {
            free_command(argv);
            continue;
        }

        if (strcmp(argv[0], "exit") == 0) {
            printf("Goodbye!\n");
            free(line);
            free_command(argv);
            return 0;
        }
        if (execute_internal(argv, &argc) == 0) {
            execute_command(argv, -1);
        }
        free_command(argv);
        argv = NULL;
    } while (true);
}

char **parse_command(char *line, int *argc) {
    const char delims[] = " \t\n";
    *argc = 0;
    bool flag = true;
    for (int i = 0; line[i] != '\0'; i++) {
        switch (line[i]) {
        case ' ':
        case '\t':
        case '\n':
            flag = true;
            break;
        default:
            if (flag) {
                (*argc)++;
                flag = false;
            }
        }
    }
    char **strings = calloc((*argc + 1), sizeof(char *));
    if (!strings) {
        perror("malloc string array");
        return strings;
    }
    char *token;

    for (int i = 0; i < *argc; i++) {
        token = strtok(i == 0 ? line : NULL, delims);
        strings[i] = calloc(strlen(token) + 1, sizeof(char));
        if (!strings[i]) {
            perror("malloc string element");
            free_command(strings);
            return NULL;
        }
        strcpy(strings[i], token);
    }
    strings[*argc] = NULL;
    return strings;
}

int execute_command(char *const argv[], int timeout_seconds) {
    pid_t pid = fork();

    switch (pid) {
    case -1:
        perror("fork");
        return 1;
    case 0:
        execvp(argv[0], argv);
        perror("execvp");
        _exit(127);
    default: {
        int status;
        pid_t result;
        do {
            result = waitpid(pid, &status, 0);
        } while (result == -1 && errno == EINTR);

        if (result == -1) {
            perror("waitpid");
            return 1;
        }
        if (WIFSIGNALED(status)) {
            printf("Child terminated by signal %d\n", WTERMSIG(status));
        } else if (WIFEXITED(status)) {
            printf("Child exited normally with status: %d\n", WEXITSTATUS(status));
        }
    }
    }
    return 0;
}

void free_command(char **argv) {
    if (argv == NULL) {
        return;
    }
    for (int i = 0; argv[i] != NULL; i++) {
        free(argv[i]);
    }
    free(argv);
}

int execute_internal(char *const argv[], int *argc) {
    if (strcmp(argv[0], "cd") == 0) {
        chdir(argv[1]);
        return 1;
    } else if (strcmp(argv[0], "pwd") == 0) {
        printf("%s\n", getcwd(NULL, 0));
        return 1;
    }
    return 0;
}
