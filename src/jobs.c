#include "jobs.h"
#include <errno.h>
#include <stdio.h>
#include <sys/wait.h>

typedef struct { pid_t pid, pgid; bool active; } Job;
static Job jobs[MAX_JOBS];

// Reap completed background children without blocking.
void reap_jobs(void) {
    int status;
    for (int i = 0; i < MAX_JOBS; ++i) {
        if (!jobs[i].active) continue;
        pid_t p = waitpid(jobs[i].pid, &status, WNOHANG);
        if (p == jobs[i].pid || (p < 0 && errno == ECHILD)) {
            printf("[done] PID %ld (PGID %ld)\n", (long)jobs[i].pid, (long)jobs[i].pgid);
            jobs[i].active = false;
        }
    }
}
// Register a child process and its process group.
void add_job(pid_t pid, pid_t pgid) {
    for (int i = 0; i < MAX_JOBS; ++i) {
        if (!jobs[i].active) { jobs[i] = (Job){pid, pgid, true}; return; }
    }
    fprintf(stderr, "shell: job table full (PID %ld)\n", (long)pid);
}
// Print background processes that are still active.
void print_jobs(void) {
    for (int i = 0; i < MAX_JOBS; ++i)
        if (jobs[i].active) printf("[running] PID %ld PGID %ld\n", (long)jobs[i].pid, (long)jobs[i].pgid);
}
