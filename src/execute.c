#include "shell.h"


int execute(char* arglist[]) {
    int status;

    // --- Step 1: Check for pipe ---
    int pipe_index = -1;
    for (int i = 0; arglist[i] != NULL; i++) {
        if (strcmp(arglist[i], "|") == 0) {
            pipe_index = i;
            break;
        }
    }

    if (pipe_index == -1) {
        // --- No pipe: single command with possible I/O redirection ---
        int cpid = fork();
        if (cpid < 0) { perror("fork failed"); exit(1); }

        if (cpid == 0) { // Child
            char* input_file = NULL;
            char* output_file = NULL;

            // Parse < and >
            for (int i = 0; arglist[i] != NULL; i++) {
                if (strcmp(arglist[i], "<") == 0 && arglist[i+1] != NULL) {
                    input_file = arglist[i+1];
                    int j = i;
                    while (arglist[j+2] != NULL) { arglist[j] = arglist[j+2]; j++; }
                    arglist[j] = NULL; arglist[j+1] = NULL;
                } else if (strcmp(arglist[i], ">") == 0 && arglist[i+1] != NULL) {
                    output_file = arglist[i+1];
                    int j = i;
                    while (arglist[j+2] != NULL) { arglist[j] = arglist[j+2]; j++; }
                    arglist[j] = NULL; arglist[j+1] = NULL;
                }
            }

            // Input redirection
            if (input_file) {
                int fd = open(input_file, O_RDONLY);
                if (fd < 0) { 
                    fprintf(stderr, "Error: cannot open input file '%s': %s\n", input_file, strerror(errno)); 
                    exit(1); 
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }

            // Output redirection
            if (output_file) {
                int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) { 
                    fprintf(stderr, "Error: cannot open output file '%s': %s\n", output_file, strerror(errno)); 
                    exit(1); 
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }

            execvp(arglist[0], arglist);
            fprintf(stderr, "Error: command not found: %s\n", arglist[0]);
            exit(1);
        } else {
            waitpid(cpid, &status, 0);
        }
        return 0;
    }

    // --- Step 2: Pipe exists ---
    char* left_cmd[pipe_index + 1];
    char* right_cmd[MAXARGS + 1];

    for (int i = 0; i < pipe_index; i++) left_cmd[i] = arglist[i];
    left_cmd[pipe_index] = NULL;

    int j = 0;
    for (int i = pipe_index + 1; arglist[i] != NULL; i++) right_cmd[j++] = arglist[i];
    right_cmd[j] = NULL;

    // --- Step 3: Parse I/O redirection on both sides ---
    char* left_input = NULL, *left_output = NULL;
    char* right_input = NULL, *right_output = NULL;

    // Left command
    for (int i = 0; left_cmd[i] != NULL; i++) {
        if (strcmp(left_cmd[i], "<") == 0 && left_cmd[i+1] != NULL) {
            left_input = left_cmd[i+1];
            int k = i;
            while (left_cmd[k+2] != NULL) { left_cmd[k] = left_cmd[k+2]; k++; }
            left_cmd[k] = NULL; left_cmd[k+1] = NULL;
        } else if (strcmp(left_cmd[i], ">") == 0 && left_cmd[i+1] != NULL) {
            left_output = left_cmd[i+1];
            int k = i;
            while (left_cmd[k+2] != NULL) { left_cmd[k] = left_cmd[k+2]; k++; }
            left_cmd[k] = NULL; left_cmd[k+1] = NULL;
        }
    }

    // Right command
    for (int i = 0; right_cmd[i] != NULL; i++) {
        if (strcmp(right_cmd[i], "<") == 0 && right_cmd[i+1] != NULL) {
            right_input = right_cmd[i+1];
            int k = i;
            while (right_cmd[k+2] != NULL) { right_cmd[k] = right_cmd[k+2]; k++; }
            right_cmd[k] = NULL; right_cmd[k+1] = NULL;
        } else if (strcmp(right_cmd[i], ">") == 0 && right_cmd[i+1] != NULL) {
            right_output = right_cmd[i+1];
            int k = i;
            while (right_cmd[k+2] != NULL) { right_cmd[k] = right_cmd[k+2]; k++; }
            right_cmd[k] = NULL; right_cmd[k+1] = NULL;
        }
    }

    // --- Step 4: Create pipe ---
    int fd[2];
    if (pipe(fd) < 0) { perror("pipe failed"); return 1; }

    // --- Step 5: Left child ---
    int left_cpid = fork();
    if (left_cpid < 0) { perror("fork failed"); return 1; }
    if (left_cpid == 0) {
        if (left_input) {
            int fd_in = open(left_input, O_RDONLY);
            if (fd_in < 0) { fprintf(stderr, "Error: cannot open input file '%s': %s\n", left_input, strerror(errno)); exit(1); }
            dup2(fd_in, STDIN_FILENO); close(fd_in);
        }
        if (left_output) {
            int fd_out = open(left_output, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd_out < 0) { fprintf(stderr, "Error: cannot open output file '%s': %s\n", left_output, strerror(errno)); exit(1); }
            dup2(fd_out, STDOUT_FILENO); close(fd_out);
        } else {
            dup2(fd[1], STDOUT_FILENO); // write to pipe
        }
        close(fd[0]); close(fd[1]);
        execvp(left_cmd[0], left_cmd);
        fprintf(stderr, "Error: command not found: %s\n", left_cmd[0]);
        exit(1);
    }

    // --- Step 6: Right child ---
    int right_cpid = fork();
    if (right_cpid < 0) { perror("fork failed"); return 1; }
    if (right_cpid == 0) {
        if (right_input) {
            int fd_in = open(right_input, O_RDONLY);
            if (fd_in < 0) { fprintf(stderr, "Error: cannot open input file '%s': %s\n", right_input, strerror(errno)); exit(1); }
            dup2(fd_in, STDIN_FILENO); close(fd_in);
        } else {
            dup2(fd[0], STDIN_FILENO); // read from pipe
        }
        if (right_output) {
            int fd_out = open(right_output, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd_out < 0) { fprintf(stderr, "Error: cannot open output file '%s': %s\n", right_output, strerror(errno)); exit(1); }
            dup2(fd_out, STDOUT_FILENO); close(fd_out);
        }
        close(fd[0]); close(fd[1]);
        execvp(right_cmd[0], right_cmd);
        fprintf(stderr, "Error: command not found: %s\n", right_cmd[0]);
        exit(1);
    }

    // --- Step 7: Parent closes pipe and waits ---
    close(fd[0]); close(fd[1]);
    waitpid(left_cpid, &status, 0);
    waitpid(right_cpid, &status, 0);

    return 0;
}

