#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/time.h>
#include <errno.h>

#define MAX_PROCESSES 477
#define MAX_ARGUMENTS 10
#define MAX_COMMAND_LENGTH 500

pid_t scheduled_pids[MAX_PROCESSES];
int process_active[MAX_PROCESSES];
char *scheduled_processes[MAX_PROCESSES][MAX_ARGUMENTS + 1]; // +1 for NULL terminator
int total_processes = 0; // Total number of processes
int current_process = -1; // Initialize to -1 to indicate no process is currently selected

// check memory allocation
void checkMemoryAllocation(void *ptr) {
    if (!ptr) {
        printf("Error: Memory allocation failed.\n");
        exit(1);
    }
}

// parse command line arguments
void parseArguments(int argc, char *argv[], int *quantum) {
    if (argc < 3) {
        printf("Usage: %s <quantum> <command1> ... <commandN>\n", argv[0]);
        exit(1);
    }

    *quantum = atoi(argv[1]);
    if (*quantum <= 0) {
        printf("Quantum must be a positive integer.\n");
        exit(1);
    }

    int argIndex = 0;

    for (int i = 2; i < argc && total_processes < MAX_PROCESSES; ++i) {
        if (strcmp(argv[i], ":") == 0) {
            scheduled_processes[total_processes][argIndex] = NULL;
            process_active[total_processes] = 1;
            total_processes++;
            argIndex = 0;
            continue;
        } else if (argIndex == 0) { // Start of a new command
            char *commandWithPrefix = malloc(strlen(argv[i]) + 3); // add space for ./ and NULL
            checkMemoryAllocation(commandWithPrefix);
            strcpy(commandWithPrefix, "./");
            strcat(commandWithPrefix, argv[i]);
            scheduled_processes[total_processes][argIndex++] = commandWithPrefix;
        } else {
            scheduled_processes[total_processes][argIndex++] = strdup(argv[i]);
            checkMemoryAllocation(scheduled_processes[total_processes][argIndex-1]);
        }

        if (i == argc - 1) {
            scheduled_processes[total_processes][argIndex] = NULL;
            process_active[total_processes] = 1;
            total_processes++;
        }
    }

    /*for (int i = 0; i < total_processes; i++) {
        //printf("argument #%d: ", i + 1);
        for (int j = 0; scheduled_processes[i][j] != NULL; j++) {
            printf("%s ", scheduled_processes[i][j]);
        }
        printf("\n");
    }*/
}

void signalHandler(int sig) {
    if (sig == SIGALRM) {
        // sigalrm -> quantum expired
        if (current_process >= 0 && scheduled_pids[current_process] > 0) {
            //printf("sigalrm\n");
            kill(scheduled_pids[current_process], SIGSTOP);
        }
    } else if (sig == SIGCHLD) {
        // sigchld -> when child process finishes
        //printf("sigchld\n");
        int status;
        pid_t pid;
        while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
            for (int i = 0; i < total_processes; i++) {
                if (scheduled_pids[i] == pid) {
                    process_active[i] = 0;
                    scheduled_pids[i] = 0;
                    break;
                }
            }
        }
    }
}

void setupTimer(int quantum) {
    struct itimerval timer;
    timer.it_value.tv_sec = quantum / 1000;
    timer.it_value.tv_usec = (quantum % 1000) * 1000;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &timer, NULL);
}

void startProcess(int index) {
    //printf("fork #%d\n", index);
    pid_t pid = fork();
    if (pid == 0) { // child process
        execvp(scheduled_processes[index][0], scheduled_processes[index]);
        // if execvp returns, there was an error
        printf("Error executing command %s\n", scheduled_processes[index][0]);
        exit(1);
    } else if (pid > 0) { // parent process
        scheduled_pids[index] = pid;
    } else {
        printf("Fork failed: %s\n", strerror(errno));
        exit(1);
    }
}

void executeRoundRobinScheduler(int quantum) {

    for (int round = 0;; round++) {
        int all_done = 1;
        for (int i = 0; i < total_processes; i++) {
            signal(SIGALRM, signalHandler); // signals for each process...
            signal(SIGCHLD, signalHandler);
            int index = (round + i) % total_processes; // round robin
            if (process_active[index]) {
                all_done = 0;
                current_process = index;
                if (scheduled_pids[index] <= 0) { // start a process
                    startProcess(index);
                } else { // resume a process
                    kill(scheduled_pids[index], SIGCONT);
                }
                setupTimer(quantum); // start timer
                pause(); // wait until sigalrm or sigchld is received. quantum expires or child process finishes
            }
        }
        if (all_done) {
            break;
        }
    }
}

int main(int argc, char *argv[]) {
    int quantum;
    // parse arguments
    parseArguments(argc, argv, &quantum);

    //run the round robin scheduler
    executeRoundRobinScheduler(quantum);

    // clean up
    for (int i = 0; i < total_processes; i++) {
        for (int j = 0; scheduled_processes[i][j] != NULL; j++) {
            free(scheduled_processes[i][j]);
        }
    }

    return 0;
}