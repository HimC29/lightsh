#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>

const char* NAME = "lightsh";
const char* VERSION = "v0.3.0";

#define MAX_LINE 1024
#define MAX_ARGS 64

struct termios orig_termios;

void disableRawMode() {
    /* Set the settings back to original settings.
     * SYNTAX: int tcsetattr(int fd, int optional_actions, const struct termios *termios_p);
     * fd is the file descriptor, always STDIN_FILENO as we are working with standard input.
     * termios_p is a pointer to a termios struct holding the settings we want to apply.
     * optional_actions is when the changes take effect.
       TCSAFLUSH is to wait for output to drain and discard any unread input.
     */
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void captureOriginalSettings() {
    if(tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
        perror("Not running in a terminal");
    }
}

void enableRawMode() {
    /* Copy the original termios settings and modify it */
    struct termios raw = orig_termios; /* The copying part */
    /* Modifying the setting
     * Clears the bits of ECHO, ICANON, and ISIG, while keeping the rest unchanged. 
     */
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    /* Apply the raw settings
     * More info about this function inside function disableRawMode
     */
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main(void) {
    /* Automatically disable raw mode when the program exits */
    atexit(disableRawMode);
    captureOriginalSettings();
    enableRawMode();
    
    char line[MAX_LINE];
    int len = 0;
    char c;
    while (1) {
        /* Reads the char the user types.
         * Stores the char to c.
         */
        ssize_t nread = read(STDIN_FILENO, &c, 1);
        /* If input stream has ended. */
        if (nread == 0) {
            return 0;
        }
        /* If theres an error in reading. */
        else if (nread == -1) {
            perror("Failed to read");
            return 1;
        }
        
        /* If user presses enter. */
        if (c == '\r' || c == '\n') {
            line[len] = '\0';
            len = 0;
            /* Return to column 0 and leave new line. */
            write(STDOUT_FILENO, "\r\n", 2);
            /* If user types exit. */
            if (strcmp(line, "exit") == 0) {
                return 0;
            }
            /* Tokenize the input. */
            char *args[MAX_ARGS]; /* Stores the arguments. */
            int i = 0; /* This variable is used to count the amount of times looped. */
            char *token = strtok(line, " "); /* Set token to the first token. */
            while (token != NULL) { /* Loop until token ends. */
               args[i] = token; /* Store the next tokens in order into args. */
               token = strtok(NULL, " "); /* Set token to the next token. */
               i++;
            }
            /* End args variable in NULL. (execvp uses it to know when it ends) */
            args[i] = NULL;
            /* Only fork/exec if there's a command to run */
            if (args[0] != NULL) {
                /* Run the command */
                pid_t pid = fork(); /* Create a twin process. */
                /* If failed to create a twin process. */
                if (pid < 0) {
                   perror("fork");
                } else if (pid == 0) {
                   /* Everything in here will be run in the child. */
                   disableRawMode();
                   execvp(args[0], args);
                   /* These runs only if execvp failed to run the command. */
                   perror("execvp");
                   exit(1);  
                } else {
                    /* Everything in here will be run in the parent. */
                    int status;
                    /* Wait until child process finishes. */
                    waitpid(pid, &status, 0);
                    /* Enable raw mode again as earlier we disabled it
                     * Avoids ECHO and write trying to write to the screen at the
                       same time
                     */
                    enableRawMode();
                }   
            }
        }
        /* If user presses backspace */
        else if (c == 127 && len > 0) {
            len--;
            /* \b is to go backwards.
             * We go back one char and write a space to overwrite the last char.
             * Then we go back one char again so the next character will overwrite
               the space.
             */
            write(STDOUT_FILENO, "\b \b", 3);
        }
        /* If user presses ctrl + d */
        else if (c == 4) {
            return 0;
        }
        /* If the user does not press enter, backspace, or ctrl + d. */
        else {
            line[len] = c;
            len++;
            /* Print the char the user types onto screen. */
            write(STDOUT_FILENO, &c, 1);
        }
    }
    return 0;
}
