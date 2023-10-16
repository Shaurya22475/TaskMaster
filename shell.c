#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>
#include <signal.h>
#include "scheduler.h"

int scheduler;
int count_processhistory=0;

#define MAX_HISTORY_SIZE 100
#define MAX_COMMAND_LENGTH 500
#define MAX_WORDS 64

typedef struct {
    char* command;
    int pid;          // Process ID
    struct timespec startTime;
    struct timespec endTime;
    double executiontime;
    int priority;
    int isTerminated; // Flag to indicate if the process has terminated
} ProcessInfo;

ProcessInfo arr[100]={0};

struct timespec endTime;
struct timespec startTime;

typedef struct CommandHistory {
    int commandNo;
    char* command;
    struct timespec startTime;
    struct timespec endTime;
    double duration;
    int background; // Flag to indicate background process
} CommandHistory;

CommandHistory history[MAX_HISTORY_SIZE]; // Array to store command history
int historySize = 0; // Number of entries in command history

// Function to print command history
void printHistory() {
    for (int i = 0; i < historySize; i++) {
        printf("Command No: %d\n", history[i].commandNo);
        printf("Command: %s\n", history[i].command);
        printf("Start Time: %ld.%09ld seconds\n", history[i].startTime.tv_sec, history[i].startTime.tv_nsec);
        printf("End Time: %ld.%09ld seconds\n", history[i].endTime.tv_sec, history[i].endTime.tv_nsec);
        printf("Duration: %.9lf seconds\n", history[i].duration);
        if (history[i].background) {
            printf("Background: Yes\n");
        }
        printf("\n");
    }
}

// Function to add a command to history 
void addToHistory(int commandNo, char* command, struct timespec startTime, struct timespec endTime, int background) {
    if (historySize < MAX_HISTORY_SIZE) {
        history[historySize].commandNo = commandNo;
        history[historySize].command = strdup(command);
        history[historySize].startTime = startTime;
        history[historySize].endTime = endTime;
        history[historySize].duration = (endTime.tv_sec - startTime.tv_sec) +
                  (double)(endTime.tv_nsec - startTime.tv_nsec) / 1e9;
        history[historySize].background = background;
        historySize++;
    } else {
        fprintf(stderr, "Command history is full. Cannot add more commands.\n");
    }
}


void sigint_handler(int signo) {
    if (signo == SIGINT) {
        for (int i=0;i<count_processhistory;i++){
            printf("%d %d",arr[0].pid,arr[1].pid);
            printf("**History**\n");
            printf("Command: %s\n",arr[i].command);
            printf("Pid: %d\n",arr[i].pid);
            printf("Priority: %d\n",arr[i].priority);
            printf("Executiontime: %.9f\n",arr[i].executiontime);
        }
    }
    exit(0);
}


void sigchld_handler(int signo) {
    int status;
    pid_t terminated_pid;
    // Wait for any child process to exit
    terminated_pid = waitpid(-1, &status, WNOHANG);
    for (int i=0;i<count_processhistory;i++){
        if (arr[i].pid == terminated_pid){
        clock_gettime(CLOCK_MONOTONIC, &arr[i].endTime);
        struct timespec executionTime;
        long seconds = arr[i].endTime.tv_sec - arr[i].startTime.tv_sec;
        long nanoseconds = arr[i].endTime.tv_nsec - arr[i].startTime.tv_nsec;

                                               // Ensure nanoseconds is positive
        if (nanoseconds < 0) {
            nanoseconds += 1000000000;       // 1 second in nanoseconds
            seconds--;                      // Subtract one second
        }

        arr[i].executiontime=seconds + (nanoseconds / 1e9);
        break;
        }
    }

    if (terminated_pid > 0 && WIFEXITED(status)) {
        // Child process has terminated
        // Send a custom signal (e.g., SIGUSR1) to the scheduler
        union sigval value;
        value.sival_int = (int)terminated_pid;
        sigqueue(scheduler, SIGUSR2, value);
    }
}

int launch(char** command, int words, int background) {
    printf("%s %s\n",command[0],command[1]);
    if (strcmp(command[0],"Submit")==0){
        ProcessInfo process;
        clock_gettime(CLOCK_MONOTONIC, &process.startTime);
         
        int length = 0;

        while (command[length] != NULL) {
            length++;
        }
        printf("length: %d",length);
        process.command=strdup(command[1]);
        int priority=1;
        if (length==3){
           priority = atoi(command[2]);
           printf("command[2]: %d",atoi(command[2]));
        }
        process.priority=priority;
        char *command_i[]={command[1],NULL};
        int pid = fork();
        if (pid<0){
            perror("fork failed!");
        }
        else if (pid==0){
            raise(SIGSTOP);
            //waiting to receive signal from scheduler.
            execvp(command[1],command_i);
        }
        //send signal to scheduler to add this newly created process to the ready queue.
        process.pid=pid;
        arr[count_processhistory]=process;
        count_processhistory++;
        union sigval value;
        value.sival_int = (int)(pid*priority);
        sigqueue(scheduler,SIGUSR1, value);
        return 0;
    }
    // Measure start time using CLOCK_MONOTONIC
    clock_gettime(CLOCK_MONOTONIC, &startTime);

    int pipes = 0;
    int pipe_indices[MAX_WORDS];

    for (int i = 0; i < words; i++) {
        if (strcmp(command[i], "|") == 0) {
            pipes++;
            pipe_indices[pipes - 1] = i;
        }
    }

    int prev_pipe_read = -1; // File descriptor to read from the previous command

    for (int i = 0; i <= pipes; i++) {
        int pipe_fd[2];

        if (i < pipes) {
            if (pipe(pipe_fd) == -1) {
                perror("Pipe failed");
                exit(EXIT_FAILURE);
            }
        }

        int status = fork();

        if (status < 0) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        } else if (status == 0) {
            // Child process
            if (i != 0) {
                // Redirect input from the previous pipe
                if (dup2(prev_pipe_read, STDIN_FILENO) == -1) {
                    perror("Dup2 failed");
                    exit(EXIT_FAILURE);
                }
                close(prev_pipe_read);
            }

            if (i != pipes) {
                // Redirect output to the current pipe
                if (dup2(pipe_fd[1], STDOUT_FILENO) == -1) {
                    perror("Dup2 failed");
                    exit(EXIT_FAILURE);
                }
                close(pipe_fd[0]);
                close(pipe_fd[1]);
            }

            // Close all pipe file descriptors in the child process
            for (int j = 0; j < pipes; j++) {
                if (j != i) {
                    close(pipe_fd[j]);
                }
            }

            int start = (i == 0) ? 0 : pipe_indices[i - 1] + 1;
            int end = (i == pipes) ? words : pipe_indices[i];
            
            char* command1[MAX_WORDS];
            int j = 0;

            for (int k = start; k < end; k++) {
                command1[j++] = command[k];
            }
            command1[j] = NULL;

            execvp(command1[0], command1);
            perror("Execvp failed");
            exit(EXIT_FAILURE);
        } else {
            // Parent process
            if (prev_pipe_read != -1) {
                close(prev_pipe_read); // Close the read end of the previous pipe
            }

            if (i != pipes) {
                close(pipe_fd[1]); // Close the write end of the current pipe
                prev_pipe_read = pipe_fd[0]; // Set the read end of the current pipe as the previous
            }

            if (!background) {
                int status2;
                waitpid(status, &status2, 0);
            }
        }
    }
    // Measure end time using CLOCK_MONOTONIC
    clock_gettime(CLOCK_MONOTONIC, &endTime);
}


int main(int argc, char* argv[]) {
    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        perror("Unable to register SIGINT handler");
        exit(1);
    }

    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);


    int NCPU=atoi(argv[1]);
    int Time_SLICE=atoi(argv[2]);
    scheduler = fork();
    if (scheduler<0){
        perror("fork failed!");
    }
    else if (scheduler==0){
        scheduler_main(NCPU,Time_SLICE);
    }
    printf("%d: scheduler pid",scheduler);
    int commandno = 1;
        do {
            char* argv[100];
            char command[MAX_COMMAND_LENGTH];
            char command_copy[MAX_COMMAND_LENGTH];
            int argc = 0;
            int count = 0;

            printf("enter command:~");
            if (fgets(command, MAX_COMMAND_LENGTH, stdin) == NULL) {
                printf("EOF happened!");
                fflush(stdout);
                break;
            }

            strcpy(command_copy, command);
            if (strcmp(command, "History\n") == 0) {
                printHistory();
                continue;
            }

            // Check if the command should run in the background
            int background = 0;
            if (command[strlen(command) - 2] == '&') {
                background = 1;
                command[strlen(command) - 2] = '\0';
            }

            char* word = strtok(command, " \t\n");
            while (word != NULL) {
                argv[count] = word;
                count++;
                argc++;
                word = strtok(NULL, " \t\n");
            }
            argv[argc] = NULL;
            launch(argv, argc, background);
            addToHistory(commandno, command_copy, startTime, endTime, background);
            commandno++;
        } while (1);
    return 0;
}
