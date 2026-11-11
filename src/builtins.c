#include "builtins.h"
#include "jobs.h"
#include "terminal.h"
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Run built-in commands inside the shell process.
int run_builtin(Pipeline *p, bool *handled) {
    *handled = false;
    if (p->count != 1 || p->background) return 0;
    Command *c = &p->commands[0];
    if (!strcmp(c->argv[0], "cd")) {
        *handled = true;
        const char *path = c->argc > 1 ? c->argv[1] : getenv("HOME");
        if (!path) path = "/";
        if (chdir(path) < 0) { perror("cd"); return 1; }
    } else if (!strcmp(c->argv[0], "exit")) {
        *handled = true;
        int code = c->argc > 1 ? atoi(c->argv[1]) : 0;
        exit(code);
    } else if (!strcmp(c->argv[0], "jobs")) {
        *handled = true;
        reap_jobs();
        print_jobs();
    } else if (!strcmp(c->argv[0], "pgid")) {
        // Inspect a process group without changing it.
        *handled = true;
        if (c->argc > 2) { fprintf(stderr, "usage: pgid [pid]\n"); return 1; }
        if (c->argc == 2) {
            char *end;
            errno = 0;
            long requested = strtol(c->argv[1], &end, 10);
            if (errno || !*c->argv[1] || *end || requested <= 0 || requested > INT_MAX) {
                fprintf(stderr, "pgid: invalid PID\n"); return 1;
            }
            pid_t found = getpgid((pid_t)requested);
            if (found < 0) { perror("getpgid"); return 1; }
            printf("PID=%ld PGID=%ld\n", requested, (long)found);
        } else {
            printf("shell PID=%ld PGID=%ld terminal foreground PGID=%ld\n",
                   (long)getpid(), (long)getpgid(0),
                   (long)(terminal_is_interactive() ? tcgetpgrp(STDIN_FILENO) : -1));
        }
    } else if (!strcmp(c->argv[0], "pwd")) {
        *handled = true;
        char *cwd = getcwd(NULL, 0);
        if (!cwd) { perror("pwd"); return 1; }
        puts(cwd); free(cwd);
    }
    return 0;
}

