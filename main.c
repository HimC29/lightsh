#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

const char* NAME = "lightsh";
const char* VERSION = "v0.1.0";

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

void enableRawMode() {
    if(tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
        perror("Not running in a terminal");
    }

    /* Automatically disable raw mode when the program exits */
    atexit(disableRawMode);

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
    enableRawMode();

    /* We need to continuously read one character at a time from standard input and
       stop when the user types q.
     * We can use a while loop to loop continuously and break to stop.
     */
    char c;
    while (1) {
        read(STDIN_FILENO, &c, 1);
        if (c == 'q')
            break;
    }

    return 0;
}
