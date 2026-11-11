#include "executor.h"
#include "builtins.h"
#include "jobs.h"
#include "terminal.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

// Connect input and output files to standard streams.
static int redirect_io(const Command *c) {
    int fd;
    if (c->input) {
        fd = open(c->input, O_RDONLY);
        if (fd < 0) { perror(c->input); return -1; }
        if (dup2(fd, STDIN_FILENO) < 0) { perror("dup2"); close(fd); return -1; }
        close(fd);
    }
    if (c->output) {
        fd = open(c->output, O_WRONLY | O_CREAT | (c->append ? O_APPEND : O_TRUNC), 0666);
        if (fd < 0) { perror(c->output); return -1; }
        if (dup2(fd, STDOUT_FILENO) < 0) { perror("dup2"); close(fd); return -1; }
        close(fd);
    }
    return 0;
}

// Restore signal behavior for child processes.
static void restore_child_signals(void) {
    struct sigaction sa = {0};
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    (void)sigaction(SIGINT, &sa, NULL);
    (void)sigaction(SIGQUIT, &sa, NULL);
    // Ignore Ctrl+Z until stopped-job tracking is supported.
    sa.sa_handler = SIG_IGN;
    (void)sigaction(SIGTSTP, &sa, NULL);
    sa.sa_handler = SIG_DFL;
    (void)sigaction(SIGTTIN, &sa, NULL);
    (void)sigaction(SIGTTOU, &sa, NULL);
}

// Group pipeline children to receive terminal signals together.
static void join_process_group(pid_t pid, pid_t pgid) {
    // Call setpgid in both processes to reduce timing races.
    if (setpgid(pid, pgid) < 0 && errno != EACCES && errno != ESRCH) {
        perror("setpgid");
    }
}

// Launch a pipeline and wait for foreground processes.
int execute(Pipeline *p) {
    bool handled;
    int rc = run_builtin(p, &handled);
    if (handled) return rc;
    int prev_read = -1;
    pid_t pids[MAX_CMDS];
    pid_t pgid = 0;
    int started = 0;
    for (int i = 0; i < p->count; ++i) {
        int fds[2] = {-1, -1};
        if (i < p->count - 1 && pipe(fds) < 0) { perror("pipe"); goto error; }
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            if (fds[0] >= 0) close(fds[0]);
            if (fds[1] >= 0) close(fds[1]);
            goto error;
        }
        if (pid == 0) {
            // Connect pipe ends and replace the child with its command.
            join_process_group(0, pgid ? pgid : getpid());
            restore_child_signals();
            if (prev_read >= 0 && dup2(prev_read, STDIN_FILENO) < 0) { perror("dup2"); _exit(126); }
            if (fds[1] >= 0 && dup2(fds[1], STDOUT_FILENO) < 0) { perror("dup2"); _exit(126); }
            if (prev_read >= 0) close(prev_read);
            if (fds[0] >= 0) close(fds[0]);
            if (fds[1] >= 0) close(fds[1]);
            if (redirect_io(&p->commands[i]) < 0) _exit(126);
            execvp(p->commands[i].argv[0], p->commands[i].argv);
            perror(p->commands[i].argv[0]);
            _exit(errno == ENOENT ? 127 : 126);
        }
        if (pgid == 0) pgid = pid;
        join_process_group(pid, pgid);
        pids[started++] = pid;
        if (prev_read >= 0) close(prev_read);
        if (fds[1] >= 0) close(fds[1]);
        prev_read = fds[0];
    }
    if (prev_read >= 0) close(prev_read);
    if (p->background) {
        // Track background children without waiting for them.
        for (int i = 0; i < started; ++i) add_job(pids[i], pgid);
        printf("[background] PGID %ld\n", (long)pgid);
        return 0;
    }

    // Give the foreground pipeline control of the terminal.
    (void)terminal_foreground(pgid);
    for (int i = 0; i < started; ++i) {
        int status;
        while (waitpid(pids[i], &status, 0) < 0) {
            if (errno == EINTR) continue;
            if (errno != ECHILD) perror("waitpid");
            break;
        }
    }
    // Return terminal control to the shell after the pipeline ends.
    (void)terminal_foreground(terminal_shell_pgid());
    return 0;
error:
    // Close inherited pipes and stop children after a launch failure.
    if (prev_read >= 0) close(prev_read);
    if (pgid > 0) (void)kill(-pgid, SIGTERM);
    for (int i = 0; i < started; ++i) {
        while (waitpid(pids[i], NULL, 0) < 0 && errno == EINTR) {}
    }
    return 1;
}
