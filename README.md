# C Shell — process group edition

A small Unix-style command interpreter built in C, designed for macOS and Linux.

## Build

```sh
make CC=clang
./csh
```

To rebuild: `make clean && make CC=clang`.

## Supported

- External commands, quotes, pipelines (`|`) and redirection (`<`, `>`, `>>`)
- Background commands (`&`) and a basic `jobs` listing
- `cd`, `pwd`, `exit`, `jobs`, `pgid` built-ins
- Each external pipeline is placed in its **own process group** using `setpgid()`
- Foreground terminal ownership is transferred with `tcsetpgrp()` and returned to the shell after the command finishes
- `Ctrl+C` interrupts the foreground pipeline, rather than terminating the shell
- `getpgid()` allows inspection of shell and child process-group IDs

## Test in an interactive terminal

```text
csh> pgid
shell PID=... PGID=... terminal foreground PGID=...
csh> printf "apple\nbanana\napple\n" | grep apple | wc -l
2
csh> sleep 30
^C
csh> sleep 10 &
[background] PGID ...
csh> jobs
[running] PID ... PGID ...
csh> pgid 12345
PID=12345 PGID=...  # replace 12345 with a real accessible process ID
```

A PGID is the identity of a **group of related processes**, distinct from the individual PID. A pipeline (e.g. `sleep 10 | cat`) uses one PGID for all of its processes. The terminal delivers Ctrl+C to the *foreground process group*.

## Limitations

This is **partial job control**, not a full POSIX interactive shell: `fg`, `bg`, and stopped-job tracking are not implemented. Ctrl+Z suspension is ignored for child commands to avoid leaving the foreground wait blocked. Command line arrow-key editing, shell variable expansion, globbing, `&&` and `||` are not implemented. The job table tracks background PIDs, not complete job status. Foreground terminal ownership is only managed in interactive mode; redirected/script input works without a controlling terminal.

On macOS, run the shell directly from Terminal or iTerm2, not from a noninteractive IDE output panel, when testing Ctrl+C and terminal control.
