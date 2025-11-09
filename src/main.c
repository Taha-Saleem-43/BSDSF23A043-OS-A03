/* main.c
 * Contains: Main loop, history management, Feature 7 (if-then-else-fi), Feature 8 (variables)
 * Features: 4 (history), 5 (semicolon), 7 (if-then-else-fi), 8 (variables)
 * Calls: shell.c (tokenize, handle_builtin), execute.c (execute)
 * Called by: OS entry point
 */

#include "shell.h"

char* history[HISTORY_SIZE];
int history_count = 0;

job_t jobs_list[MAX_JOBS];
int jobs_count = 0;

// ============ HISTORY FUNCTIONS (Feature 4) ============

void add_to_history(const char* cmdline) {
    if (history_count < HISTORY_SIZE) {
        history[history_count] = strdup(cmdline);
    } else {
        free(history[0]);
        for (int i = 1; i < HISTORY_SIZE; i++) {
            history[i-1] = history[i];
        }
        history[HISTORY_SIZE - 1] = strdup(cmdline);
    }
    history_count++;
}

int handle_bang_command(char** cmdline_ptr) {
    char* cmdline = *cmdline_ptr;
    if (cmdline[0] == '!') {
        int n = 0;
        for (int i = 1; cmdline[i] != '\0'; i++) {
            if (!isdigit(cmdline[i])) {
                fprintf(stderr, "Invalid !n command\n");
                return 0;
            }
            n = n * 10 + (cmdline[i] - '0');
        }
        if (n < 1 || n > history_count) {
            fprintf(stderr, "No such command in history: !%d\n", n);
            return 0;
        }
        free(*cmdline_ptr);
        *cmdline_ptr = strdup(history[n - 1]);
        printf("%s\n", *cmdline_ptr);
        return 1;
    }
    return 0;
}

// ============ FEATURE 7: IF-THEN-ELSE-FI ============

int is_if_statement(const char* cmd) {
    if (cmd == NULL) return 0;
    while (*cmd == ' ' || *cmd == '\t') cmd++;
    if (strncmp(cmd, "if", 2) == 0) {
        char next = cmd[2];
        if (next == '\0' || next == ' ' || next == '\t') {
            return 1;
        }
    }
    return 0;
}

int is_keyword(const char* line, const char* keyword) {
    while (*line == ' ' || *line == '\t') line++;
    int klen = strlen(keyword);
    if (strncmp(line, keyword, klen) == 0) {
        char next = line[klen];
        if (next == '\0' || next == ' ' || next == '\t' || next == '\n') {
            return 1;
        }
    }
    return 0;
}

void trim_string(char* str) {
    char* start = str;
    while (*start == ' ' || *start == '\t') start++;
    
    char* end = start + strlen(start) - 1;
    while (end >= start && (*end == ' ' || *end == '\t' || *end == '\n')) {
        *end = '\0';
        end--;
    }
    
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
}

int read_if_block(if_block_t* block) {
    char buffer[MAX_LEN];
    int in_then = 0, in_else = 0;
    
    block->then_count = 0;
    block->else_count = 0;
    block->condition_cmd = NULL;
    
    printf("if> ");
    fflush(stdout);
    
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        fprintf(stderr, "Error: unexpected EOF in if block\n");
        return 0;
    }
    
    char* cond = buffer;
    while (*cond == ' ' || *cond == '\t') cond++;
    if (strncmp(cond, "if", 2) == 0) {
        cond += 2;
        while (*cond == ' ' || *cond == '\t') cond++;
    }
    
    trim_string(cond);
    block->condition_cmd = strdup(cond);
    
    in_then = 0;
    in_else = 0;
    
    while (1) {
        printf("if> ");
        fflush(stdout);
        
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            fprintf(stderr, "Error: unexpected EOF in if block\n");
            return 0;
        }
        
        trim_string(buffer);
        
        if (is_keyword(buffer, "then")) {
            in_then = 1;
            in_else = 0;
            continue;
        } else if (is_keyword(buffer, "else")) {
            in_then = 0;
            in_else = 1;
            continue;
        } else if (is_keyword(buffer, "fi")) {
            return 1;
        }
        
        if (in_then && block->then_count < MAX_BLOCK_LINES) {
            block->then_block[block->then_count] = strdup(buffer);
            block->then_count++;
        } else if (in_else && block->else_count < MAX_BLOCK_LINES) {
            block->else_block[block->else_count] = strdup(buffer);
            block->else_count++;
        } else if (!in_then && !in_else) {
            fprintf(stderr, "Error: commands must come after 'then' or 'else'\n");
            return 0;
        }
    }
    
    return 1;
}

int execute_condition(const char* cmd) {
    if (cmd == NULL || cmd[0] == '\0') {
        return 1;
    }
    
    char** arglist = tokenize((char*)cmd);
    if (arglist == NULL) {
        return 1;
    }
    
    // Feature 8: Expand variables in condition
    arglist = expand_variables(arglist);
    
    int status;
    int cpid = fork();
    
    if (cpid < 0) {
        perror("fork failed");
        return 1;
    }
    
    if (cpid == 0) {
        execvp(arglist[0], arglist);
        exit(127);
    } else {
        waitpid(cpid, &status, 0);
        
        for (int i = 0; arglist[i] != NULL; i++)
            free(arglist[i]);
        free(arglist);
        
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }
        return 1;
    }
}

void execute_block(char** block, int count) {
    for (int i = 0; i < count; i++) {
        if (block[i] == NULL || block[i][0] == '\0') continue;
        
        char** arglist = tokenize(block[i]);
        if (arglist != NULL) {
            // Feature 8: Expand variables before checking builtin
            arglist = expand_variables(arglist);
            
            if (!handle_builtin(arglist)) {
                execute(arglist);
            }
            for (int j = 0; arglist[j] != NULL; j++)
                free(arglist[j]);
            free(arglist);
        }
    }
}

int execute_if_block(if_block_t* block) {
    if (block->condition_cmd == NULL) {
        fprintf(stderr, "Error: no condition in if block\n");
        return 1;
    }
    
    int exit_status = execute_condition(block->condition_cmd);
    
    if (exit_status == 0) {
        execute_block(block->then_block, block->then_count);
    } else {
        if (block->else_count > 0) {
            execute_block(block->else_block, block->else_count);
        }
    }
    
    if (block->condition_cmd) free(block->condition_cmd);
    for (int i = 0; i < block->then_count; i++) {
        if (block->then_block[i]) free(block->then_block[i]);
    }
    for (int i = 0; i < block->else_count; i++) {
        if (block->else_block[i]) free(block->else_block[i]);
    }
    
    return 0;
}

int handle_if_statement(char* cmdline) {
    if_block_t block;
    
    if (read_if_block(&block)) {
        return execute_if_block(&block);
    }
    
    return 1;
}

// ============ MAIN LOOP (Features 5 - Semicolon) ============

int main() {
    char* cmdline;
    char** arglist;
    
    initialize_readline();

    while ((cmdline = readline(PROMPT)) != NULL) {
        reap_background_jobs();
        
        if (*cmdline == '\0') {
            free(cmdline);
            continue;
        }

        char* cmd_ptr = cmdline;
        char* command;
        
        while ((command = strsep(&cmd_ptr, ";")) != NULL) {
            while (*command == ' ' || *command == '\t') command++;
            
            char* end = command + strlen(command) - 1;
            while (end > command && (*end == ' ' || *end == '\t')) {
                *end = '\0';
                end--;
            }
            
            if (*command == '\0') continue;

            char* cmd_copy = strdup(command);
            handle_bang_command(&cmd_copy);
            add_to_history(cmd_copy);
            add_history(cmd_copy);

            // Feature 8: Check for variable assignment
            if (is_assignment(cmd_copy)) {
                // Parse assignment
                char* equal_pos = strchr(cmd_copy, '=');
                int name_len = equal_pos - cmd_copy;
                char name[ARGLEN];
                strncpy(name, cmd_copy, name_len);
                name[name_len] = '\0';
                
                char* value = equal_pos + 1;
                // Remove quotes if present
                if ((value[0] == '"' && value[strlen(value)-1] == '"') ||
                    (value[0] == '\'' && value[strlen(value)-1] == '\'')) {
                    value++;
                    char* last = strchr(value, '\0') - 1;
                    *last = '\0';
                }
                
                set_variable(name, value);
            }
            // Feature 7: Check if this is an if statement
            else if (is_if_statement(cmd_copy)) {
                handle_if_statement(cmd_copy);
            } else if ((arglist = tokenize(cmd_copy)) != NULL) {
                // Feature 8: Expand variables in command
                arglist = expand_variables(arglist);
                
                if (!handle_builtin(arglist)) {
                    execute(arglist);
                }
                for (int i = 0; arglist[i] != NULL; i++)
                    free(arglist[i]);
                free(arglist);
            }
            
            free(cmd_copy);
        }
        
        free(cmdline);
    }

    free_all_variables();
    printf("\nShell exited.\n");
    return 0;
}
