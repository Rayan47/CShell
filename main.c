#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

char **parse_command(char *line, int *argc);

int execute_command(char *const argv[], int timeout_seconds);

int execute_internal(char *const argv[]);

void free_command(char **argv);

int main(void) {

    printf("Shell Initialized <3\n");

    char *line = NULL;
    size_t size = 0; // bytes
    ssize_t n_chars = 0;
    int argc;
    char **argv = NULL;

    do {
        if (line != NULL) {
            free(line);
            line = NULL;
        }
        if (argv != NULL) {
            free_command(argv);
            argv = NULL;
        }
        argc = 0;
        char *cwd = getcwd(NULL, 0);
        if (cwd == NULL) {
            perror("getcwd");
            printf("shell>");
        } else {
            printf("shell@%s>", cwd);
            free(cwd);
        }
        fflush(stdout);

        n_chars = getline(&line, &size, stdin);

        if (feof(stdin)) {
            printf("Goodbye!\n");
            free(line);
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
            continue;
        }

        if (strcmp(argv[0], "exit") == 0) {
            printf("Goodbye!\n");
            free(line);
            free_command(argv);
            argv = NULL;
            return 0;
        }
        if (execute_internal(argv) == 0) {
            execute_command(argv, -1);
        }
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

int execute_internal(char *const argv[]) {
    if (strcmp(argv[0], "cd") == 0) {
        char *path;
        if (argv[1] == NULL) {
            path = getenv("HOME");
        } else {
            path = argv[1];
        }
        int res = chdir(path);
        if (res == -1) {
            perror("cd");
            return 3;
        }
        return 1;
    } else if (strcmp(argv[0], "pwd") == 0) {
        char *cwd = getcwd(NULL, 0);
        if (cwd == NULL) {
            perror("getcwd");
            return 2;
        }
        printf("%s\n", cwd);
        free(cwd);
        return 1;
    }
    return 0;
}
