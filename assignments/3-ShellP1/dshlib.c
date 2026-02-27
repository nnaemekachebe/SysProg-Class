#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "dshlib.h"

//===================================================================
// HELPER FUNCTIONS - Memory Management (PROVIDED)
//===================================================================

/**
 * alloc_cmd_buff - Allocate memory for cmd_buff internal buffer
 * 
 * This function is provided for you. It allocates the _cmd_buffer
 * that will store the command string.
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
 * 
 * This function is provided for you. Call it when done with a cmd_buff.
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
 * 
 * This function is provided for you.
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
 * 
 * This function is provided for you. It frees all allocated memory
 * in a command_list_t structure.
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
// PARSING FUNCTIONS - YOU IMPLEMENT THESE
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
 * 
 * This function takes a single command string (no pipes) and parses
 * it into argc/argv format. It handles quoted strings to preserve
 * spaces within them.
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
 * 
 * This function:
 *   1. Checks if input is empty/whitespace only
 *   2. Splits input by pipe character '|'
 *   3. For each segment, creates a cmd_buff_t
 *   4. Stores all cmd_buffs in command_list_t
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
    if (strcmp(input, "dragon") == 0) {
        return BI_CMD_DRAGON;
    }
    if (strcmp(input, "cd") == 0) {
        return BI_CMD_CD;
    }
    return BI_NOT_BI;
}

// Dragon ASCII art - Extra Credit
static const char *dragon_art[] = {
"                                                                        @%%%%                       ",
"                                                                     %%%%%%                         ",
"                                                                    %%%%%%                          ",
"                                                                 % %%%%%%%           @              ",
"                                                                %********%        %*******           ",
"                                       %%%%%%%  %%%%@         %***********@    %******  @****        ",
"                                  %*********************      ************************          ",
"                                *************************   *********** ****************           ",
"                               ****************************** ********************     ***            ",
"                             ******************************* @*******************        **            ",
"                            ********************************** %***********************                ",
"                            *************************************************************               ",
"                            **********************************@*******@              ",
"      %*******@           %**************        ******************************      **                ",
"    **************         %%@*************           ************ **************      @%                ",
"  ***********   ***        ***************            ****************************                        ",
" **********       *         **************             ***********@***************                        ",
"**********                * %*************            @******************************                      ",
"*********                 * *@*************            @**********************************                  ",
"*******@                   ***************           %*********************************              ",
"**********                  ***************          %**********************************      ****   ",
"*********@                   @**************         ************@ **** %****************   *********",
"**********                  *****************        **************      ****************** *********",
"*********@**@                ***************@       ***************     ***********************  **",
" **********                  * %***************@        **************   ************************** **",
"  ************  @           *******************        ****************************************  *** ",
"   ************** **  %  %@ *******************          **************************************    *** ",
"    ************************** ******************           @*******************************    ****** ",
"     ***************************************              %*******************************        ***  ",
"      @***********************************                  %***************************               ",
"        **********************************                      ********************  *******          ",
"           **************************                           **************@ **********         ",
"              ********************           @%@%                  @*******************   ***        ",
"                  **************        **********                    ***************    %         ",
"                ************************************                      **************            ",
"                **************************  %%%% ***                      **********  ***@          ",
"                     ******************* %***** %%                          ************@          ",
"                                                                                 %*******@       "
};

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
            // Exit is handled in main loop
            return BI_CMD_EXIT;
            
        case BI_CMD_DRAGON:
            // Extra credit - print the dragon
            for (int i = 0; i < (int)(sizeof(dragon_art) / sizeof(dragon_art[0])); i++) {
                printf("%s\n", dragon_art[i]);
            }
            return BI_EXECUTED;
            
        case BI_CMD_CD:
            // CD will be implemented in Part 2
            printf("cd not implemented yet!\n");
            return BI_EXECUTED;
            
        default:
            return BI_NOT_BI;
    }
}

//===================================================================
// MAIN SHELL LOOP - YOU IMPLEMENT THIS
//===================================================================

/**
 * exec_local_cmd_loop - Main shell loop
 * 
 * This function:
 *   1. Loop forever (while(1))
 *   2. Print the shell prompt (SH_PROMPT)
 *   3. Read a line of input using fgets()
 *   4. Remove trailing newline
 *   5. Check for exit command - if found, print "exiting..." and break
 *   6. Parse the command line using build_cmd_list()
 *   7. Handle return codes appropriately
 *   8. Print the parsed commands in required format
 *   9. Free the command list using free_cmd_list()
 *   10. Loop back to step 2
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
        
        // Check for dragon command (extra credit)
        if (strcmp(cmd_line, "dragon") == 0) {
            // Execute dragon command
            for (int i = 0; i < (int)(sizeof(dragon_art) / sizeof(dragon_art[0])); i++) {
                printf("%s\n", dragon_art[i]);
            }
            continue;
        }
        
        // Parse the command line
        rc = build_cmd_list(cmd_line, &clist);
        
        // Handle return codes
        if (rc == WARN_NO_CMDS) {
            printf("%s", CMD_WARN_NO_CMD);
            continue;
        } else if (rc == ERR_TOO_MANY_COMMANDS) {
            printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
            continue;
        } else if (rc != OK) {
            // Other error - just continue
            continue;
        }
        
        // Print the parsed commands in required format
        printf(CMD_OK_HEADER, clist.num);
        
        // Print each command
        for (int i = 0; i < clist.num; i++) {
            printf("<%d> %s", i + 1, clist.commands[i].argv[0]);
            
            // If there are arguments, print them in brackets
            if (clist.commands[i].argc > 1) {
                printf(" [");
                for (int j = 1; j < clist.commands[i].argc; j++) {
                    printf("%s", clist.commands[i].argv[j]);
                    if (j < clist.commands[i].argc - 1) {
                        printf(" ");
                    }
                }
                printf("]");
            }
            printf("\n");
        }
        
        // Free memory
        free_cmd_list(&clist);
    }
    
    return OK;
}

//===================================================================
// EXECUTION FUNCTIONS - For future assignments
//===================================================================

/**
 * exec_cmd - Execute a single command (Part 2)
 */
int exec_cmd(cmd_buff_t *cmd)
{
    (void)cmd;  // Suppress unused parameter warning
    printf("exec_cmd not implemented yet (Part 2)\n");
    return OK;
}

/**
 * execute_pipeline - Execute piped commands (Part 3)
 */
int execute_pipeline(command_list_t *clist)
{
    (void)clist;  // Suppress unused parameter warning
    printf("execute_pipeline not implemented yet (Part 3)\n");
    return OK;
}

