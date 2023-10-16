#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include "scheduler.h"
#define _XOPEN_SOURCE 700
#define MAX_QUEUE_SIZE 100
#define SIG_ADD_PROCESS SIGUSR1
#define MAX_PROCESSES 100
typedef struct {
    int pid;          // Process ID
    struct timespec startTime;
    struct timespec endTime;
    int nooftimeslices;
    int priority;
    int isTerminated; // Flag to indicate if the process has terminated
} ProcessInfo;

typedef struct {
    int front, rear, size;
    ProcessInfo array[MAX_QUEUE_SIZE];
} CircularQueue;

int fd;

CircularQueue* readyQueue1;
CircularQueue* readyQueue2;
CircularQueue* readyQueue3;
CircularQueue* readyQueue4;

ProcessInfo executingProcesses[MAX_PROCESSES]={0};  

// Function to initialize the process structure
ProcessInfo createProcessInfo(int pid) {
    ProcessInfo process;
    process.pid = pid;
    clock_gettime(CLOCK_MONOTONIC, &process.startTime);
    process.endTime.tv_sec = 0;
    process.endTime.tv_nsec = 0;
    process.isTerminated = 0;
    return process;
}
CircularQueue* initQueue() {
    CircularQueue* queue = (CircularQueue*)malloc(sizeof(CircularQueue));
    if (queue == NULL) {
        fprintf(stderr, "Memory allocation failed for the queue.\n");
        exit(EXIT_FAILURE);
    }
    queue->front = queue->rear = -1;
    queue->size = 0;
    return queue;
}

int min(int a, int b){
    if (a >= b){
        return b;
    }
    return a;
}

int isEmpty(CircularQueue* queue) {
    return (queue->size == 0);
}

int isFull(CircularQueue* queue) {
    return (queue->size == MAX_QUEUE_SIZE);
}

void enqueue(CircularQueue* queue, ProcessInfo *process) {
    if (isFull(queue)) {
        fprintf(stderr, "Queue is full. Cannot enqueue.\n");
        return;
    }

    if (isEmpty(queue)) {
        queue->front = queue->rear = 0;
        queue->array[queue->rear] = *process;
    } else {
        queue->rear = (queue->rear + 1) % MAX_QUEUE_SIZE;
        queue->array[queue->rear] = *process;
    }

    queue->size++;
}
ProcessInfo dequeue(CircularQueue* queue) {
    ProcessInfo emptyProcess; // A placeholder for an empty process
    emptyProcess.pid = 0;    // Set PID to 0 to indicate an empty slot

    if (isEmpty(queue)) {
        fprintf(stderr, "Queue is empty. Cannot dequeue.\n");
        return emptyProcess;
    }
    ProcessInfo frontProcess = queue->array[queue->front];
    if (queue->front == queue->rear) {
        queue->front = queue->rear = -1;
    } else {
        queue->front = (queue->front + 1) % MAX_QUEUE_SIZE;
    }
    queue->size--;
    return frontProcess;
}

void scheduleProcess(ProcessInfo* process) {
    // Send SIGCONT to start the process
    kill(process->pid, SIGCONT);


    process->isTerminated = 0; // Mark the process as not terminated
    printf("Scheduled process with PID %d\n", process->pid);
}

void stopProcess(ProcessInfo* process) {
    // Send SIGSTOP to stop the process
    kill(process->pid, SIGSTOP);
    printf("Stopped process with PID %d\n", process->pid);
}

// void displayQueue(CircularQueue* queue) {
//     if (isEmpty(queue)) {
//         printf("Queue is empty.\n");
//     } else {
//         printf("Queue contents: ");
//         int i = queue->front;
//         while (i != queue->rear) {
//             printf("PID %d, ", queue->array[i]);
//             i = (i + 1) % MAX_QUEUE_SIZE;
//         }
//         printf("PID %d\n", queue->array[i]);
//     }
// }

// int main() {
//     CircularQueue* readyQueue = initQueue();

//     enqueue(readyQueue, 1);
//     enqueue(readyQueue, 2);

//     // Dequeue and process PIDs
//     while (!isEmpty(readyQueue)) {
//         int frontPid = dequeue(readyQueue);
//         printf("Dequeued: PID %d\n", frontPid);
//     }

//     // Continue enqueuing, dequeuing, and processing as needed

//     free(readyQueue);
//     return 0;
// }

void addProcessHandler(int signo, siginfo_t *info, void *context) {
    if (signo == SIG_ADD_PROCESS) {
        int pidtimespriority = info->si_int;
        printf("Pidtimesprority from shell: %d",pidtimespriority);
        int sch = getpid();
        printf("The scheduler pid: %d",sch);
        //Just remember to make the necessary change in the
        int prior = (int)(pidtimespriority/sch);
        int pid = (int)pidtimespriority/prior;
        ProcessInfo* process = (ProcessInfo*)malloc(sizeof(ProcessInfo));
        process->pid = pid;
        process->isTerminated = 0; // Mark the process as not terminated
        process->nooftimeslices = 0;
        process->priority=prior;
        printf("Process with pid:%d has priority %d\n",pid,prior);
        // Record the start time
        if (clock_gettime(CLOCK_MONOTONIC, &(process->startTime)) == -1) {
            perror("clock_gettime");
        }

        // Enqueue the process into the appropriate priority queue
        if (process->priority == 1) {
            enqueue(readyQueue1, process);
        } else if (process->priority == 2) {
            enqueue(readyQueue2, process);
        } else if (process->priority == 3) {
            enqueue(readyQueue3, process);
        } else if (process->priority == 4) {
            enqueue(readyQueue4, process);
        } else {
            // Handle invalid priority
            fprintf(stderr, "Invalid priority for PID %d\n", pid);
            free(process);
        }
    }
}


void terminationHandler(int signo, siginfo_t *info, void *context) {
    printf("ARFREE BHAI MUJHE BHI LO!\n");
    int terminated_pid = info->si_value.sival_int;
    printf("Process %d termination signal has been caught!",terminated_pid);

    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (executingProcesses[i].pid!=-1){
            if (executingProcesses[i].pid==terminated_pid){
                executingProcesses[i].isTerminated=1;
                if (clock_gettime(CLOCK_MONOTONIC, &(executingProcesses[i].endTime)) == -1) {
                      perror("clock_gettime");
                }
                printf("Process with PID %d has terminated.\n", terminated_pid);
                break;
            }
        }
    }
}


void scheduler_main(int NCPU,int TSLICE){
    struct sigaction sa_add_process; // Variable for handling SIG_ADD_PROCESS
    sa_add_process.sa_flags = SA_SIGINFO;
    sa_add_process.sa_sigaction = addProcessHandler;
    sigaction(SIG_ADD_PROCESS, &sa_add_process, NULL);

    struct sigaction sa_termination; // Variable for handling SIGUSR2
    sa_termination.sa_flags = SA_SIGINFO;
    sa_termination.sa_sigaction = terminationHandler;
    sigaction(SIGUSR2, &sa_termination, NULL);

    readyQueue1 = initQueue();
    readyQueue2 = initQueue();
    readyQueue3 = initQueue();
    readyQueue4 = initQueue();

    int z=0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
            executingProcesses[i].pid = -1;
            executingProcesses[i].isTerminated=-1;
    // You may set other fields to their default values here if needed
    }   

    while (1){
            // For all available CPU resources
            int i;
            int count = 0;
           for (i = 0; i < NCPU; i++) {
            // Try to dequeue from the highest-priority queue first
            ProcessInfo pid;

            if (!isEmpty(readyQueue4)) {
                pid = dequeue(readyQueue4);
            } else if (!isEmpty(readyQueue3)) {
                pid = dequeue(readyQueue3);
            } else if (!isEmpty(readyQueue2)) {
                pid = dequeue(readyQueue2);
            } else if (!isEmpty(readyQueue1)) {
                pid = dequeue(readyQueue1);
            } else {
                break;  // No more processes to schedule
            }

            scheduleProcess(&pid);
            executingProcesses[i] = pid;
            count++;
        }

            fflush(stdout);

            int k=sleep(TSLICE);
            while (k>0){
                k = sleep(k);
            }

            fflush(stdout);
        
            // Stop the processes and add them back to the ready queue
            
    for (i = 0; i < min(NCPU, count); i++) {
        stopProcess(&executingProcesses[i]);

        if (!executingProcesses[i].isTerminated) {
            // Enqueue the process back to its respective priority queue
            int priority = executingProcesses[i].priority;
            switch (priority) {
                case 1:
                    enqueue(readyQueue1, &executingProcesses[i]);
                    break;
                case 2:
                    enqueue(readyQueue2, &executingProcesses[i]);
                    break;
                case 3:
                    enqueue(readyQueue3, &executingProcesses[i]);
                    break;
                case 4:
                    enqueue(readyQueue4, &executingProcesses[i]);
                    break;
                // Handle additional priorities as needed
            }
        }

                    executingProcesses[i].pid = -1;
                    executingProcesses[i].isTerminated = -1;
        }
        fflush(stdout);
              // Sleep for the time quantum (TSLICE) while waiting for processes to execute.          // Sleep for the time quantum.
    }
}
