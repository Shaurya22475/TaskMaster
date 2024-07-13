
TaskMaster

This project is a simple process scheduler written in C. It handles process scheduling based on priority and time slices, and supports multiple priority queues.


## Features
    Multiple Priority Queues: Four priority levels to manage processes.
    Signal Handling: Uses signals for process management and communication.
    Command History: Maintains a history of commands executed.
## Prerequisites

    A C compiler (like gcc).
## Installing and Setup

- Navigate to your desired directory on your system
- Clone the repository (git clone /repositoryurl/)
- Compile the executable (gcc -o TaskMaster scheduler.c shell.c)
- Run the TaskMaster executable with the arguments (./TaskMaster <No.CPUs> <TimeSlice>)

## Usage/Examples

The shell would run and prompt the user to enter commands

Commands are any executable, also as an argument you can add the priority of a task, priorities can be from 1 - 4. 

this argument is optional and default is 1.

Examples : 

- ls 3
Puts the ls executable to be run with a priority of 3

- ./fib
Puts the ./fib executable to be run with a default priority of 1.
