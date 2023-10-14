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
    int front, rear, size;
    int array[MAX_QUEUE_SIZE];
} CircularQueue;
int fd;
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

CircularQueue* readyQueue;

int isEmpty(CircularQueue* queue) {
    return (queue->size == 0);
}

int isFull(CircularQueue* queue) {
    return (queue->size == MAX_QUEUE_SIZE);
}

void enqueue(CircularQueue* queue, int pid) {
    if (isFull(queue)) {
        fprintf(stderr, "Queue is full. Cannot enqueue.\n");
        return;
    }

    if (isEmpty(queue)) {
        queue->front = queue->rear = 0;
        queue->array[queue->rear] = pid;
    } else {
        queue->rear = (queue->rear + 1) % MAX_QUEUE_SIZE;
        queue->array[queue->rear] = pid;
    }

    queue->size++;
}

int dequeue(CircularQueue* queue) {
    if (isEmpty(queue)) {
        fprintf(stderr, "Queue is empty. Cannot dequeue.\n");
        return -1;
    }

    int frontPid = queue->array[queue->front];

    if (queue->front == queue->rear) {
        queue->front = queue->rear = -1;
    } else {
        queue->front = (queue->front + 1) % MAX_QUEUE_SIZE;
    }

    queue->size--;

    return frontPid;
}

void writeQueueToFile(const char* filename, CircularQueue* queue) {
    int fd = open(filename, O_WRONLY | O_CREAT, 0666);
    write(fd,"Let's see",sizeof("Let's see"));
    if (fd == -1) {

    }

    if (isEmpty(queue)) {
        write(fd, "Queue is empty.\n", strlen("Queue is empty.\n"));
    } else {
        char buffer[256]; // Adjust the buffer size as needed
        int i = queue->front;
        int length = 0;

        while (i != queue->rear) {
            length = snprintf(buffer, sizeof(buffer), "PID %d, ", queue->array[i]);
            write(fd, buffer, length);
            i = (i + 1) % MAX_QUEUE_SIZE;
        }

        length = snprintf(buffer, sizeof(buffer), "PID %d\n", queue->array[i]);
        write(fd, buffer, length);
    }

    close(fd);
}

void scheduleProcess(int pid) {
    // Send SIGCONT to start the process
    kill(pid, SIGCONT);
    printf("Scheduled process with PID %d\n", pid);
}

void stopProcess(int pid) {
    // Send SIGSTOP to stop the process
    kill(pid, SIGSTOP);
    printf("Stopped process with PID %d\n", pid);
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
    write(fd,"\naur kuch batoa!!",sizeof("\naur kuch batoa!!"));
    if (signo == SIG_ADD_PROCESS) {
        int pid = info->si_int;
        enqueue(readyQueue, pid);
    }
    writeQueueToFile("/home/flameblazer/Documents/SimpleShell/new.txt",readyQueue);
}

void scheduler_main(int NCPU,int TSLICE){
    int fd = open("/home/flameblazer/Documents/SimpleShell/new.txt", O_WRONLY | O_CREAT, 0666);
    write(fd,"hellobhai",sizeof("hellobhai"));
    printf("are yaar");
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = addProcessHandler;
    sigaction(SIG_ADD_PROCESS, &sa, NULL);
    readyQueue = initQueue();
    int z=0;
    int executingProcesses[MAX_PROCESSES] = {0};
    while (1){
        if (isEmpty(readyQueue)) {
            // No processes in the ready queue, so sleep for the time quantum
            sleep(TSLICE * 1000);
        } 
        else {
            // For all available CPU resources
            sleep(TSLICE * 1000);
            int i;
            for (i = 0; i < NCPU && !isEmpty(readyQueue); i++) {
                int pid = dequeue(readyQueue);
                scheduleProcess(pid);
                executingProcesses[i] = pid;
            }
            fflush(stdout);
            sleep(TSLICE * 1000);
            for (int i=readyQueue->front;i<readyQueue->size;i++){
                     printf("PID: %d\n",readyQueue->array[i]);
            }
            fflush(stdout);
            // Stop the processes and add them back to the ready queue
            for (i = 0; i < NCPU; i++) {
                if (executingProcesses[i] != 0) {
                    stopProcess(executingProcesses[i]);
                    enqueue(readyQueue, executingProcesses[i]);
                    executingProcesses[i] = 0;
                }
            }
            fflush(stdout);
        }
    }
            // Sleep for the time quantum (TSLICE) while waiting for processes to execute.
// Sleep for the time quantum.
}
    
