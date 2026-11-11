#include "terminal.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

static bool interactive;
static pid_t shell_pgid;

// Detect the terminal and initialize the shell process group.
void terminal_init(void) {
    interactive = isatty(STDIN_FILENO);
    shell_pgid = getpgrp();
    if (!interactive) return;

    // Only take terminal control when launched in the foreground.
    if (tcgetpgrp(STDIN_FILENO) != shell_pgid) {
        fprintf(stderr, "csh: must be launched as a foreground job\n");
        interactive = false;
        return;
    }
    // macOS may already assign the shell its own process group.
    if (setpgid(0, 0) < 0 && errno != EPERM && errno != EACCES) {
        perror("setpgid(shell)");
    }
    shell_pgid = getpgrp();
    // Verify that the shell owns the foreground terminal.
    if (tcgetpgrp(STDIN_FILENO) != shell_pgid) {
        fprintf(stderr, "csh: shell does not own foreground terminal\n");
        interactive = false;
    }
}

// Report whether terminal job control is available.
bool terminal_is_interactive(void) { return interactive; }
// Return the shell process group ID.
pid_t terminal_shell_pgid(void) { return shell_pgid; }
// Assign terminal foreground control to a process group.
int terminal_foreground(pid_t pgid) {
    if (!interactive) return 0;
    if (tcsetpgrp(STDIN_FILENO, pgid) < 0) {
        perror("tcsetpgrp"); return -1;
    }
    return 0;
}
