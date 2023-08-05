#include <unistd.h>
#include <stdio.h>
#include <cstdarg>

#include <sys/ptrace.h>
#include <sys/wait.h>

void run_program(const char* programname);
void count_instructions_in(pid_t child_pid);
void printWithPid(const char* format, ...);

int main(int argc, char** argv)
{
    if (argc < 2) {
        fprintf(stderr, "Expected a program name as argument\n");
        return -1;
    }
    char * programName = argv[1];

    // Start process to debug in new thread
    pid_t child_pid = fork();
    if (child_pid == 0)
        run_program(programName);
    else if (child_pid > 0)
        count_instructions_in(child_pid);
    else {
        perror("Fork failed");
        return -1;
    }

    return 0;
}

void run_program(const char* programname)
{
    printWithPid("target started. will run '%s'\n", programname);

    /* Allow tracing of this process */
    if (ptrace(PTRACE_TRACEME, 0, 0, 0) < 0) {
        perror("ptrace");
        return;
    }

    /* Execute program */
    execl(programname, programname, NULL);
}

void count_instructions_in(pid_t child_pid)
{
    printWithPid("debugger started\n");

    /* Wait for child to stop on its first instruction */
    int wait_status;
    wait(&wait_status);

    unsigned icounter = 0;
    while (WIFSTOPPED(wait_status)) {
        icounter++;
        /* Make the child execute another instruction */
        if (ptrace(PTRACE_SINGLESTEP, child_pid, 0, 0) < 0) {
            perror("ptrace");
            return;
        }

        /* Wait for child to stop on its next instruction */
        wait(&wait_status);
    }

    printWithPid("the child executed %u instructions\n", icounter);
}

void printWithPid(const char* format, ...)
{
    // va_list is used to access the variable number of arguments.
    // This is denoted by the ellipsis '...'
    va_list ap;
    fprintf(stdout, "[%d] ", getpid());
    va_start(ap, format);
    vfprintf(stdout, format, ap);
    va_end(ap);
}
