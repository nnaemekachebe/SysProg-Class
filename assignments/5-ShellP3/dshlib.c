#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>

#include "dshlib.h"

// Global variable to store last return code (for extra credit)
static int last_return_code = 0;

//===================================================================
// HELPER FUNCTIONS - Memory Management (PROVIDED)
//===================================================================

/**
 * alloc_cmd_buff - Allocate memory for cmd_buff internal buffer
 */
int alloc_cmd_buff(cmd_buff_t *cmd_buff)
{
    cmd_buff->_cmd_buffer = malloc(SH_CMD_MAX);
    if (cmd_buff->_cmd_buffer == NULL) {
        return ERR_MEMORY;
    }
    cmd_buff->argc = 0;
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    return OK;
}

/**
 * free_cmd_buff - Free cmd_buff internal buffer
 */
int free_cmd_buff(cmd_buff_t *cmd_buff)
{
    if (cmd_buff->_cmd_buffer != NULL) {
        free(cmd_buff->_cmd_buffer);
        cmd_buff->_cmd_buffer = NULL;
    }
    cmd_buff->argc = 0;
    return OK;
}

/**
 * clear_cmd_buff - Reset cmd_buff without freeing memory
 */
int clear_cmd_buff(cmd_buff_t *cmd_buff)
{
    cmd_buff->argc = 0;
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    return OK;
}

/**
 * free_cmd_list - Free all cmd_buffs in a command list
 */
int free_cmd_list(command_list_t *cmd_lst)
{
    for (int i = 0; i < cmd_lst->num; i++) {
        free_cmd_buff(&cmd_lst->commands[i]);
    }
    cmd_lst->num = 0;
    return OK;
}

//===================================================================
// PARSING FUNCTIONS
//===================================================================

/**
 * Helper function to trim leading and trailing whitespace
 */
static char *trim_whitespace(char *str)
{
    while (isspace((unsigned char)*str)) {
        str++;
    }
    
    if (*str == '\0') {
        return str;
    }
    
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        end--;
    }
    
    *(end + 1) = '\0';
    return str;
}

/**
 * build_cmd_buff - Parse a single command string into cmd_buff_t
 */
int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff)
{
    // Check for NULL inputs
    if (cmd_line == NULL || cmd_buff == NULL) {
        return ERR_MEMORY;
    }
    
    // Trim leading/trailing whitespace from input
    char *trimmed = trim_whitespace(cmd_line);
    
    // Check if empty after trimming
    if (*trimmed == '\0') {
        cmd_buff->argc = 0;
        return OK;
    }
    
    // Copy the trimmed string to the internal buffer
    strncpy(cmd_buff->_cmd_buffer, trimmed, SH_CMD_MAX - 1);
    cmd_buff->_cmd_buffer[SH_CMD_MAX - 1] = '\0';
    
    // Parse the command line into tokens with quote handling
    int argc = 0;
    char *buffer = cmd_buff->_cmd_buffer;
    int input_len = strlen(buffer);
    int pos = 0;
    
    while (argc < CMD_ARGV_MAX - 1 && pos < input_len) {
        // Skip whitespace
        while (pos < input_len && (buffer[pos] == ' ' || buffer[pos] == '\t')) {
            pos++;
        }
        
        if (pos >= input_len) {
            break;
        }
        
        // Check for quoted strings
        char *arg_start;
        
        if (buffer[pos] == '"') {
            // Double-quoted string
            pos++; // Skip opening quote
            arg_start = &buffer[pos];
            
            // Find closing quote
            while (pos < input_len && buffer[pos] != '"') {
                pos++;
            }
            
            // Null-terminate at closing quote
            if (pos < input_len) {
                buffer[pos] = '\0';
                pos++; // Skip closing quote
            }
        } else if (buffer[pos] == '\'') {
            // Single-quoted string
            pos++; // Skip opening quote
            arg_start = &buffer[pos];
            
            // Find closing quote
            while (pos < input_len && buffer[pos] != '\'') {
                pos++;
            }
            
            // Null-terminate at closing quote
            if (pos < input_len) {
                buffer[pos] = '\0';
                pos++; // Skip closing quote
            }
        } else {
            // Regular argument - find end
            arg_start = &buffer[pos];
            while (pos < input_len && buffer[pos] != ' ' && buffer[pos] != '\t') {
                pos++;
            }
            
            // Null-terminate at end of argument
            if (pos < input_len) {
                buffer[pos] = '\0';
                pos++;
            }
        }
        
        // Store the argument
        cmd_buff->argv[argc] = arg_start;
        argc++;
    }
    
    // Set argc and NULL terminate the argv array
    cmd_buff->argc = argc;
    cmd_buff->argv[argc] = NULL;
    
    return OK;
}

/**
 * build_cmd_list - Parse command line with pipes into command_list_t
 */
int build_cmd_list(char *cmd_line, command_list_t *clist)
{
    // Check for NULL inputs
    if (cmd_line == NULL || clist == NULL) {
        return WARN_NO_CMDS;
    }
    
    // Trim leading/trailing whitespace
    char *trimmed = trim_whitespace(cmd_line);
    
    // Check if empty after trimming
    if (*trimmed == '\0') {
        clist->num = 0;
        return WARN_NO_CMDS;
    }
    
    // Make a copy of the input since strtok modifies the string
    char *cmd_copy = malloc(strlen(trimmed) + 1);
    if (cmd_copy == NULL) {
        return ERR_MEMORY;
    }
    strcpy(cmd_copy, trimmed);
    
    // Count pipe characters to check for too many commands
    int pipe_count = 0;
    char *p = cmd_copy;
    while ((p = strchr(p, PIPE_CHAR)) != NULL) {
        pipe_count++;
        p++;
    }
    
    // Check if too many commands (more than CMD_MAX)
    if (pipe_count >= CMD_MAX) {
        free(cmd_copy);
        clist->num = 0;
        return ERR_TOO_MANY_COMMANDS;
    }
    
    // Initialize command list
    clist->num = 0;
    
    // Split by pipe character
    char *saveptr;  // For strtok_r
    char *segment = strtok_r(cmd_copy, "|", &saveptr);
    
    while (segment != NULL && clist->num < CMD_MAX) {
        // Trim whitespace from this segment
        char *trimmed_segment = trim_whitespace(segment);
        
        // Allocate cmd_buff for this command
        int rc = alloc_cmd_buff(&clist->commands[clist->num]);
        if (rc != OK) {
            free_cmd_list(clist);
            free(cmd_copy);
            return ERR_MEMORY;
        }
        
        // Parse the segment into cmd_buff
        rc = build_cmd_buff(trimmed_segment, &clist->commands[clist->num]);
        if (rc != OK) {
            free_cmd_list(clist);
            free(cmd_copy);
            return rc;
        }
        
        // Only count non-empty commands
        if (clist->commands[clist->num].argc > 0) {
            clist->num++;
        }
        
        // Get next segment
        segment = strtok_r(NULL, "|", &saveptr);
    }
    
    free(cmd_copy);
    
    // Handle empty command list
    if (clist->num == 0) {
        return WARN_NO_CMDS;
    }
    
    return OK;
}

//===================================================================
// BUILT-IN COMMAND FUNCTIONS
//===================================================================

/**
 * match_command - Check if input is a built-in command
 */
Built_In_Cmds match_command(const char *input)
{
    if (input == NULL) {
        return BI_NOT_BI;
    }
    if (strcmp(input, EXIT_CMD) == 0) {
        return BI_CMD_EXIT;
    }
    if (strcmp(input, CD_CMD) == 0) {
        return BI_CMD_CD;
    }
    if (strcmp(input, DRAGON_CMD) == 0) {
        return BI_CMD_DRAGON;
    }
    return BI_NOT_BI;
}

/**
 * exec_built_in_cmd - Execute built-in commands
 */
Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd)
{
    if (cmd == NULL || cmd->argv[0] == NULL) {
        return BI_NOT_BI;
    }
    
    Built_In_Cmds bi_cmd = match_command(cmd->argv[0]);
    
    switch (bi_cmd) {
        case BI_CMD_EXIT:
            return BI_CMD_EXIT;
            
        case BI_CMD_DRAGON:
            #ifdef DRAGON_IMPLEMENTED
            print_dragon();
            #else
            printf("Dragon not implemented\n");
            #endif
            return BI_EXECUTED;
            
        case BI_CMD_CD:
            // CD built-in: change directory
            if (cmd->argc < 2) {
                // No argument - do nothing (stay in current directory)
                return BI_EXECUTED;
            }
            if (chdir(cmd->argv[1]) != 0) {
                perror("cd");
            }
            return BI_EXECUTED;
            
        default:
            return BI_NOT_BI;
    }
}

//===================================================================
// EXECUTION FUNCTIONS
//===================================================================

/**
 * exec_cmd - Execute single command using fork/exec
 */
int exec_cmd(cmd_buff_t *cmd)
{
    if (cmd == NULL || cmd->argv[0] == NULL) {
        return ERR_EXEC_CMD;
    }
    
    // Fork the process
    pid_t pid = fork();
    
    // Check fork result
    if (pid < 0) {
        // Fork failed
        perror("fork");
        return ERR_EXEC_CMD;
    }
    else if (pid == 0) {
        // Child process - execute command
        execvp(cmd->argv[0], cmd->argv);
        
        // If execvp returns, it failed
        if (errno == ENOENT) {
            fprintf(stderr, "Command not found in PATH\n");
            exit(127);
        }
        else if (errno == EACCES) {
            fprintf(stderr, "Permission denied\n");
            exit(126);
        }
        else {
            perror("execvp");
            exit(ERR_EXEC_CMD);
        }
    }
    else {
        // Parent process - wait for child
        int status;
        waitpid(pid, &status, 0);
        
        // Extract exit code
        if (WIFEXITED(status)) {
            last_return_code = WEXITSTATUS(status);
            return last_return_code;
        }
        return ERR_EXEC_CMD;
    }
}

/**
 * execute_pipeline - Execute piped commands
 */
int execute_pipeline(command_list_t *clist)
{
    if (clist == NULL || clist->num == 0) {
        return ERR_EXEC_CMD;
    }
    
    int num_commands = clist->num;
    int num_pipes = num_commands - 1;
    
    // Handle single command case (shouldn't happen, but safety check)
    if (num_pipes == 0) {
        return exec_cmd(&clist->commands[0]);
    }
    
    // Create pipe file descriptors array
    int pipe_fds[num_pipes][2];
    
    // Step 1: Create all pipes
    for (int i = 0; i < num_pipes; i++) {
        if (pipe(pipe_fds[i]) < 0) {
            perror("pipe");
            return ERR_EXEC_CMD;
        }
    }
    
    // Array to store child PIDs
    pid_t pids[num_commands];
    
    // Step 2: Fork and setup each command
    for (int i = 0; i < num_commands; i++) {
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("fork");
            // Close all pipes before returning
            for (int j = 0; j < num_pipes; j++) {
                close(pipe_fds[j][0]);
                close(pipe_fds[j][1]);
            }
            return ERR_EXEC_CMD;
        }
        
        if (pid == 0) {  // Child process
            // Setup stdin from previous pipe (if not first command)
            if (i > 0) {
                dup2(pipe_fds[i-1][0], STDIN_FILENO);
            }
            
            // Setup stdout to next pipe (if not last command)
            if (i < num_commands - 1) {
                dup2(pipe_fds[i][1], STDOUT_FILENO);
            }
            
            // Close ALL pipe file descriptors
            for (int j = 0; j < num_pipes; j++) {
                close(pipe_fds[j][0]);
                close(pipe_fds[j][1]);
            }
            
            // Execute command
            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            
            // If execvp returns, it failed
            if (errno == ENOENT) {
                fprintf(stderr, "Command not found in PATH\n");
                exit(127);
            }
            else if (errno == EACCES) {
                fprintf(stderr, "Permission denied\n");
                exit(126);
            }
            else {
                perror("execvp");
                exit(ERR_EXEC_CMD);
            }
        }
        
        // Parent process - store child PID
        pids[i] = pid;
    }
    
    // Step 3: Parent closes all pipes (critical!)
    for (int i = 0; i < num_pipes; i++) {
        close(pipe_fds[i][0]);
        close(pipe_fds[i][1]);
    }
    
    // Step 4: Wait for all children
    for (int i = 0; i < num_commands; i++) {
        int status;
        waitpid(pids[i], &status, 0);
        
        // Store exit code of last command
        if (WIFEXITED(status)) {
            last_return_code = WEXITSTATUS(status);
        }
    }
    
    return OK;
}

//===================================================================
// MAIN SHELL LOOP
//===================================================================

/**
 * exec_local_cmd_loop - Main shell loop
 */
int exec_local_cmd_loop()
{
    char cmd_line[SH_CMD_MAX];
    command_list_t clist;
    int rc;
    
    while (1) {
        // Print the shell prompt
        printf("%s", SH_PROMPT);
        
        // Read a line of input
        if (fgets(cmd_line, SH_CMD_MAX, stdin) == NULL) {
            // EOF received (Ctrl+D)
            printf("\n");
            break;
        }
        
        // Remove trailing newline
        cmd_line[strcspn(cmd_line, "\n")] = '\0';
        
        // Check for exit command
        if (strcmp(cmd_line, EXIT_CMD) == 0) {
            printf("exiting...\n");
            break;
        }
        
        // Parse into command_list_t
        rc = build_cmd_list(cmd_line, &clist);
        
        // Handle return codes
        if (rc == WARN_NO_CMDS) {
            printf("%s", CMD_WARN_NO_CMD);
            continue;
        } else if (rc == ERR_TOO_MANY_COMMANDS) {
            printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
            continue;
        } else if (rc != OK) {
            continue;
        }
        
        // Check if first command is built-in
        if (clist.num > 0) {
            Built_In_Cmds bi = exec_built_in_cmd(&clist.commands[0]);
            
            if (bi == BI_CMD_EXIT) {
                printf("exiting...\n");
                free_cmd_list(&clist);
                break;
            }
            if (bi == BI_EXECUTED) {
                free_cmd_list(&clist);
                continue;
            }
        }
        
        // Execute command(s)
        if (clist.num == 1) {
            // Single command - use exec_cmd
            exec_cmd(&clist.commands[0]);
        } else {
            // Pipeline - use execute_pipeline
            execute_pipeline(&clist);
        }
        
        // Free memory
        free_cmd_list(&clist);
    }
    
    return OK;
}

