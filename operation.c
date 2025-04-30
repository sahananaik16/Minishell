/* Including the header file which contains function declarations and macros */
#include "header.h"

/* Array of built-in shell commands */
char *builtins[] = {"echo", "printf", "read", "cd", "pwd", "pushd", "popd", "dirs", "let", "eval",
                    "set", "unset", "export", "declare", "typeset", "readonly", "getopts", "source",
                    "exit", "exec", "shopt", "caller", "true", "type", "hash", "bind", "help", "fg", "bg", "jobs", "NULL"};

/* Global variable to store the shell prompt */
extern char prompt[100];

/* Array to store external commands read from a file */
char *external_cmds[200];

/* Global variable to store user input */
extern char input_string[100];

/* Process ID and status variables */
int pid = 0, status;

/* Pointer to the head of a singly linked list */
slist *head = NULL;

/* Function to scan and process user input */
void scan_input(char *prompt, char *input_string)
{
    /* Handling signals for interruption and stopping */
    signal(SIGINT, signal_handler);
    signal(SIGTSTP, signal_handler);

    /* Extract external commands from a file */
    extract_external_commands(external_cmds);
    
    /* Infinite loop to continuously take input */
    while (1)
    {
        /* Display the prompt and take input */
        printf("%s", prompt);
        scanf("%[^\n]", input_string);
        getchar();
        
        /* If user inputs "PS1", change the prompt */
        if (!strcmp(input_string, "PS1"))
        {
            strcpy(prompt, "PS1");
            continue;   
        }
        
        /* Extract command from input string */
        char *cmd = get_command(input_string);
        
        /* Check if command is built-in or external */
        int ret = check_command_type(cmd);
        
        /* If command is external, create a child process */
        if (ret == EXTERNAL)
        {
            pid = fork();

            /* Parent process waits for child */
            if (pid > 0)
            {
                waitpid(pid, &status, WUNTRACED);
                pid = 0;
            }

            /* Child process executes the command */
            else if (pid == 0)
            {
                signal(SIGINT, SIG_DFL);
                signal(SIGTSTP, SIG_DFL);

                execute_external_commands(input_string);
            }
        }
        /* If command is built-in, execute it directly */
        else if (ret == BUILTIN)
        {
            execute_internal_commands(input_string, cmd);
        }
    }
}

/* Function to extract external commands from a file */
void extract_external_commands(char **external_commands)
{
    /* Open the file in read mode */
    int fd = open("command.txt", O_RDONLY);
    if (fd == -1)
    {
        perror("OPEN");
        return;
    }

    /* Variables for reading and storing commands */
    char ch, buf[100];
    int buf_flag = 0, ext = 0;

    /* Read file character by character */
    while (read(fd, &ch, 1))
    {
        /* If character is not a newline, store it */
        if (ch != '\n')
        {
            buf[buf_flag++] = ch;
        }
        /* If newline is encountered, store the command */
        else
        {
            external_cmds[ext] = calloc(buf_flag, sizeof(char));

            /* Check for memory allocation failure */
            if (external_cmds[ext] == NULL)
            {
                perror("dynamic memory allocation failed\n");
                return;
            }
            
            /* Copy command into allocated memory */
            strcpy(external_cmds[ext], buf);
            
            /* Clear buffer and reset flags */
            memset(buf, 0, sizeof(buf));
            buf_flag = 0;
            ext++;
        }
    }
}

char *get_command(char *input_string)
{
    /** Buffer to store the command temporarily */
    char buff[20];
    /** Loop counters */
    int i = 0, j = 0;
    /** Iterate through the input string */
    while (input_string[i] != '\0')
    {
        /** If a space is encountered, it marks the end of the command */
        if (input_string[i] == ' ')
        {
            /** Null-terminate the buffer */
            buff[j] = '\0';
            /** Allocate memory for the command string */
            char *cmd = (char *)malloc(strlen(buff) + 1);
            /** Check if memory allocation was successful */
            if (cmd == NULL)
            {
                printf("Error: Memory allocation failed.\n");
                return NULL;
            }
            /** Copy the command from the buffer to the allocated memory */
            strcpy(cmd, buff);
            return cmd;
        }
        else
        {
            /** Copy the character to the buffer */
            buff[j++] = input_string[i];
        }
        i++;
    }

    /** If no space is encountered, the entire string is the command */
    buff[j] = '\0';
    char *cmd = (char *)malloc(strlen(buff) + 1);
    if (cmd == NULL)
    {
        printf("Error: Memory allocation failed.\n");
        return NULL;
    }
    strcpy(cmd, buff);
    return cmd;
}


int check_command_type(char *command)
{
    /** Loop counter for built-in commands */
    int i = 0;
    /** Iterate through the built-in commands array */
    while (strcmp(builtins[i], "NULL"))
    {
        /** If the command matches a built-in command, return BUILTIN */
        if (!strcmp(builtins[i], command))
            return BUILTIN;
        i++;
    }

    /** Reset loop counter for external commands */
    i = 0;

    /** Iterate through the external commands array */
    while (external_cmds[i])
    {
        /** If the command matches an external command, return EXTERNAL */
        if (!strcmp(external_cmds[i], command))
            return EXTERNAL;
        i++;
    }
    /** If no match is found, return NO_COMMAND */
    return NO_COMMAND;
}

/**
 * Executes an external command.
 * @param input_string The user input string.
 */
void execute_external_commands(char *input_string)
{
    /** Array to store command arguments */
    char *argv[100], ptr[100];
    /** Loop counters */
    int count = 0, k = 0;
    /** Iterate through the input string to parse arguments */
    for (int i = 0; i < (strlen(input_string) + 1); i++)
    {
        /** If a space or end of string is encountered, it marks the end of an argument */
        if (input_string[i] == ' ' || input_string[i] == '\0')
        {
            /** Null-terminate the argument string */
            ptr[count] = '\0';
            count = 0;

            /** Allocate memory for the argument */
            argv[k] = malloc(strlen(ptr) + 1);

            /** Check if memory allocation was successful */
            if (argv[k] == NULL)
            {
                printf("Dynamic memory allocation failed\n");
                return;
            }

            /** Copy the argument to the allocated memory */
            strcpy(argv[k++], ptr);
        }
        else
        {
            /** Copy the character to the argument buffer */
            ptr[count++] = input_string[i];
        }
    }
    /** Null-terminate the argument list */
    argv[k] = NULL;

    /** Check for pipes in the command */
    if (strchr(input_string, '|'))
    {
        /** Execute the command with pipes */
        npipe(input_string);
        exit(0);
    }
    else
    {
        /** Execute the external command using execvp */
        execvp(argv[0], argv);
    }
}

/**
 * Executes a built-in command.
 * @param input_string The user input string.
 * @param cmd The command name.
 */
void execute_internal_commands(char *input_string, char *cmd)
{
    /** Buffers for various purposes */
    char buff[100], path[100], j = 0;

    /** Check for built-in commands and execute them */
    if (!strcmp(input_string, "exit"))
    {
        exit(0); // Exit the shell
    }
    else if (!strcmp(input_string, "pwd"))
    {
        getcwd(buff, 100); // Get current working directory
        printf("%s\n", buff); // Print the current working directory
    }
    else if (!strcmp(cmd, "cd"))
    {
        int flag_slash = 0;
        /** Extract the path from the input string */
        for (int i = 0; input_string[i] != '\0'; i++)
        {
            if (!flag_slash)
            {
                if (input_string[i] == '/')
                {
                    flag_slash = 1;
                    path[j++] = '/';
                }
            }
            else
            {
                path[j++] = input_string[i];
            }
        }
        path[j] = '\0';
        printf("%s\n", path); // Print the path (for debugging)

        /** Change the current directory */
        if (chdir(path) != 0)
            printf("error\n"); // Print error if chdir fails
    }
    else if (!strcmp(cmd, "echo"))
    {
        int flag_dlr = 0;
        /** Extract the argument for echo (handling $ variables) */
        for (int i = 0; input_string[i] != '\0'; i++)
        {
            if (!flag_dlr)
            {
                if (input_string[i] == '$')
                {
                    flag_dlr = 1;
                }
            }
            else
            {
                path[j++] = input_string[i];
            }
        }
        path[j] = '\0';

        /** Handle special echo cases ($ for PID, ? for exit status, SHELL) */
        if (!strcmp(path, "$"))
        {
            printf("%d\n", getpid()); // Print the process ID
        }
        else if (!strcmp(path, "?"))
        {
            int wstatus;
            if (WIFEXITED(wstatus))
                printf("Child terminated normally with exit status %d\n", wstatus);
            else
                printf("Child terminated abnormally with exit status %d\n", wstatus);
        }
        else if (!strcmp(path, "SHELL"))
        {
            printf("%s\n", getenv("SHELL")); // Print the shell environment variable
        } else {
             printf("%s\n", path); // regular echo command
        }
    }
    else if(!strcmp(cmd, "fg"))
    {
        fg(); // Bring a background job to the foreground
        
        
    }
    else if(!strcmp(cmd, "bg"))
    {
        bg(); // Continue a stopped background job
    
    }
    else if(!strcmp(cmd, "jobs"))
    {
        print_list(); // Print the list of background jobs
    }
}

/**
 * Handles pipes in external commands.
 * @param input_str The user input string containing the pipe command.
 */
void npipe(char *input_str)
{
    /** Tokenize the input string to get individual commands and arguments */
    char *word = strtok(input_str, " ");

    /** Array to store all the words (commands and arguments) */
    int count = 0;
    char *argv[100];
    argv[count++] = word;

    /** Tokenize the rest of the input string */
    while (1)
    {
        word = strtok(NULL, " ");

        if (!word)
            break;

        argv[count++] = word;
    }
    argv[count] = NULL; // Null-terminate the argv array

    /** Count the number of commands (separated by '|') */
    int i = 0, cmd_count = 0;
    while (argv[i])
    {
        if (!strcmp(argv[i], "|"))
        {
            cmd_count++;
        }

        i++;
    }
    cmd_count++; // Increment for the last command

    /** Array to store the starting index of each command in argv */
    i = 0;
    int cmd[cmd_count], j = 0;
    cmd[j++] = 0; // First command starts at index 0
    while (argv[i])
    {
        if (!strcmp(argv[i], "|"))
        {
            cmd[j++] = i + 1; // Next command starts after the '|'
            argv[i] = NULL;    // Null-terminate the previous command's argument list
        }
        i++;
    }

    /** File descriptor for the pipe */
    int fd[2];

    /** Iterate through each command to be piped */
    for (int i = 0; i <= cmd_count; i++)
    {
        /** Create a pipe except for the last command */
        if (i != cmd_count - 1)
        {
            pipe(fd);
        }

        /** Fork a new process for each command */
        int ret = fork();

        /** Child process: Execute the command */
        if (ret == 0)
        {
            /** Redirect input from the previous command's output (if not the first command) */
            if (i != cmd_count - 1)
            {
                close(fd[0]);      // Close read end of the pipe
                dup2(fd[1], 1);     // Redirect stdout to the write end of the pipe
            }
            /** Execute the command */
            execvp(argv[cmd[i]], argv + cmd[i]);
        }
        /** Parent process: Prepare for the next command */
        else
        {
            /** Redirect input from the previous command's output (if not the first command) */
            if (i != cmd_count - 1)
            {
                close(fd[1]);      // Close write end of the pipe
                dup2(fd[0], 0);     // Redirect stdin to the read end of the pipe
                close(fd[0]);      // Close read end of the pipe
            }
            /** Wait for the child process to finish */
            wait(NULL);
        }
    }
}

/**
 * Signal handler for SIGINT (Ctrl+C) and SIGTSTP (Ctrl+Z).
 * @param signum The signal number.
 */
void signal_handler(int signum)
{
    /** Handle SIGINT */
    if (signum == SIGINT)
    {
        if (pid == 0) // If it's the foreground process
        {
            printf("\n%s", prompt); // Print the prompt again
            fflush(stdout);       // Flush the output buffer
        }
    }
    /** Handle SIGTSTP */
    else if (signum == SIGTSTP)
    {
        if (pid == 0) // If it's the foreground process
        {
            printf("\n%s", prompt); // Print the prompt again
            fflush(stdout);       // Flush the output buffer
        }
        else // If it's a background process
        {
            insert_at_last(); // Add the process to the background job list
        }
    }
}

int insert_at_last()
{
    slist *new = malloc(sizeof(slist));
    /* Check whether new node created or not */
    if (new == NULL)
    {
        return -1;
    }

    /* Fill the parts of the node */
    new->pid = pid;
    strcpy(new->cmd, input_string);
    new->link = NULL;

    /* If list is empty */
    if (head == NULL)
    {
        /*If *head is empty then create the first node */
        head = new;
        return 0;
    }
    slist *temp = head;
    /*traverse through the list*/
    while (temp->link)
    {
        temp = temp->link;
    }
    temp->link = new;
    return 0;
}

void print_list()
{
    /*if list is empty*/
    if (head == NULL)
    {
        /*print error message*/
        printf("INFO : No jobs in the list\n");
    }
    /*if list is not empty*/
    else
    {
        /*traverse trhough loop in forward direction and print data*/
        slist *temp=head;
        while (temp)
        {
            printf("pid %d,cmd %s\n", temp->pid, temp->cmd);
            temp = temp->link;
        }
    }
}


/**
 * Brings the most recent background job to the foreground.
 */
void fg() {
    /** Print the command of the job to be brought to foreground */
    printf("%s\n",head->cmd);
    /** Check if there are any background jobs */
    if (head == NULL) {
        printf("No jobs in background\n");
        return;
    }

    /** Traverse the job list to find the last (most recent) job */
    slist *temp = head;
    slist *prev = NULL;

    while (temp->link != NULL) {
        prev = temp;
        temp = temp->link;
    }

    /** Send SIGCONT signal to the job to continue it */
    kill(temp->pid, SIGCONT);

    /** Wait for the foreground job to complete */
    waitpid(temp->pid, NULL, 0); // Use 0 for no options

    /** Remove the job from the background job list */
    if (prev) {
        prev->link = NULL; // Remove from the middle of the list
    } else {
        head = NULL; // Remove the only element in the list
    }
    free(temp); // Free the memory of the removed job node
}

/**
 * Continues a stopped background job.
 */
void bg() {
    /** Check if there are any background jobs */
    if (head == NULL) {
        printf("No jobs in background\n");
        return;
    }

     /** Traverse the job list to find the last (most recent) job */
    slist *temp = head;
    slist *prev = NULL;

    while (temp->link != NULL) {
        prev = temp;
        temp = temp->link;
    }


    /** Send SIGCONT signal to the job to continue it in the background */
    kill(temp->pid, SIGCONT);
    

    /** Remove the job from the background job list */
    if (prev) {
        prev->link = NULL; // Remove from the middle of the list
    } else {
        head = NULL; // Remove the only element in the list
    }
    free(temp); // Free the memory of the removed job node
}