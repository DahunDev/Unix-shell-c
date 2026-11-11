#include "shell.h"
#include "lexer.h"
#include "parser.h"
#include "executor.h"
#include "jobs.h"
#include "terminal.h"
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static volatile sig_atomic_t interactive_shell = 0;

// Cancel interactive input without terminating the shell.
static void handle_sigint(int signo) {
    (void)signo;
    // Use async-signal-safe write() to move to a new prompt line.
    if (interactive_shell) {
        const char newline = '\n';
        (void)write(STDOUT_FILENO, &newline, 1);
    }
}

// Configure signals and run the command read-execute loop.
int main(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; // Let getline return EINTR on Ctrl+C.
    if (sigaction(SIGINT, &sa, NULL) < 0) { perror("sigaction"); return 1; }
    struct sigaction ignore;
    memset(&ignore, 0, sizeof(ignore));
    ignore.sa_handler = SIG_IGN;
    sigemptyset(&ignore.sa_mask);
    if (sigaction(SIGQUIT, &ignore, NULL) < 0) { perror("sigaction"); return 1; }
    // Prevent terminal-control signals from suspending the shell.
    struct sigaction stop_ignore;
    memset(&stop_ignore, 0, sizeof(stop_ignore));
    stop_ignore.sa_handler = SIG_IGN;
    sigemptyset(&stop_ignore.sa_mask);
    (void)sigaction(SIGTTOU, &stop_ignore, NULL);
    (void)sigaction(SIGTTIN, &stop_ignore, NULL);
    (void)sigaction(SIGTSTP, &stop_ignore, NULL);
    terminal_init();
    char *line = NULL;
    size_t cap = 0;
    bool interactive = terminal_is_interactive();
    interactive_shell = interactive ? 1 : 0;
    // Read, tokenize, parse, and execute one command line at a time.
    while (1) {
        reap_jobs();
        if (interactive) { printf("csh> "); fflush(stdout); }
        ssize_t len = getline(&line, &cap, stdin);
        if (len < 0) {
            if (errno == EINTR) {
                clearerr(stdin);
                continue;
            }
            break;
        }
        Token tokens[MAX_TOKENS];
        int n = tokenize(line, tokens);
        if (n < 0) continue;
        Pipeline p;
        if (parse(tokens, n, &p) < 0) fprintf(stderr, "shell: invalid syntax\n");
        else if (p.count) execute(&p);
        free_tokens(tokens, n);
    }
    free(line);
    return 0;
}
