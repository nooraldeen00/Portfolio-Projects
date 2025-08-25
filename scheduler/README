In this assignment you will be implementing a priority scheduler, the user program _ps_, a number of system calls, and a test application.

You must use a GitHub Codespace to develop this assignment.

## Reading
To prepare yourself for the assignment read the following sections from the [xv6 book](documentation\book-riscv-rev3.pdf):

- 2.2 User mode, supervisor mode, and system calls
- 2.3 Kernel organization
- 2.4 Code: xv6 organization
- 2.5 Process overview
- 2.6: Code: starting xv6, the first process and system call

- 4.1 RISC-V trap machinery
- 4.2 Traps from user space
- 4.3 Code: Calling system calls
- 4.4 Code: System call arguments
- 4.5 Traps from kernel space

- All of section 7 Scheduling

## Process Control Block
You will modify the process control block to include the effective priority and real priority of a process.  The priority must be in the p->lock section of the process.  All processes default to a priority of 0. 

### Hint
Every time these two values are read or set then the p->lock must be acquired and then released.

## System Calls

### int setPrority( int priority )
You will implement a system call that will set the priority of a process.  Valid priority levels are -20 to 20.  You will store the priority of a process in its process control block as the process's real priority.  This routine should return 0 if successful, and -1 otherwise (if, for example, the caller passes in an invalid prority).

### int setEffectivePriority( int pid, int priority )
This system call sets the effective priority of the process with the given pid. This routine should return 0 if successful, and -1 otherwise (if, for example, the caller passes in an invalid pid or a priority < -20 or > 20.

### int getpinfo(struct pstat *)
The second is int getpinfo(struct pstat *). This routine returns some information about all running processes, including how many times each has been chosen to run and the process ID of each. You will use this system call to build a variant of the command line program ps, which can then be called to see what is going on. The structure pstat is defined below; note, you cannot change this structure, and must use it exactly as is. This routine should return 0 if successful, and -1 otherwise (if, for example, a bad or NULL pointer is passed into the kernel).

You'll need to understand how to fill in the structure pstat in the kernel and pass the results to user space. The structure should look like what you see here, in a file you'll have to include called pstat.h:

```c
#ifndef _PSTAT_H_
#define _PSTAT_H_

#include "param.h"

struct pstat {
  char name[NPROC][16];        // name of the process
  enum procstate state[NPROC]; // state of the process   
  int inuse[NPROC];            // whether this slot of the process table is in use (1 or 0)
  int effective_priority       // the effective priority of the process
  int real_priority            // the real priority of the process
  int pid[NPROC];              // the PID of each process
  int ticks[NPROC];            // the number of ticks each process has accumulated 
};

#endif // _PSTAT_H_

```

### Hint

Good examples of how to pass arguments into the kernel are found in existing system calls. In particular, follow the path of read(), which will lead you to sys_read(), which will show you how to use argptr() (and related calls) to obtain a pointer that has been passed into the kernel. Note how careful the kernel is with pointers passed from user space -- they are a security threat, and thus must be checked very carefully before usage. Use either_copyout() to copy data from kernel space to user space.  You can find it in process.c For either_copyout, if user_dst==1, then dst is a user virtual address (user space) otherwise, dst is a kernel address.

## ps

Your ps application will print the following:

```
NAME    PID     STATUS      PRIORITY    
init    1       SLEEPING    1     
sh      2       SLEEPING    1  
test    4       SLEEPING    -10      
ps      6       RUNNING     20  
```
### Hint

Copy one of the existing use programs such as wc.c to use as a framework for your user application.

## The scheduler

You will change the round robin scheduler in proc.c void scheduler(void) to be a priority scheduler.  Most of the code for the scheduler is quite localized and can be found in proc.c; the associated header file, proc.h is also quite useful to examine. To change the scheduler, not much needs to be done; study its control flow and then try some small changes.  If the running process has a priority tie with another process in the queue then the non-running process will run.

### Aging 

If a process has not run for 10 ticks of the scheduler at its current effective priority level then you will raise the effective priority level by 1.  Once a process runs you will set its effective priority back to its real priority.


## Graph and Test Application

You'll have to make a graph for this assignment. The graph should show the number of time slices a set of three processes receives over time, where the processes have a priority of -20, 0, and 20. The graph is likely to be pretty boring, but should clearly show that your scheduler works as desired.  The graph must be submitted as a PDF file at the top level of your repo.

To gather this data you will need to write an application that forks three children and each child runs and prints their pid.  From the console output you can determine the number of times each ran.

## SUBMITTING

Push all your changes to your main branch.  

## BUILDING AND RUNNING XV6

### To build the kernel:
```
make
```

### To build the userspace applications and run the OS
```
make qemu
```

### To exit xv6
```
ctrl-a x
```

## Administrative

This assignment must be coded in C. Any other language will result in 0 points. Your programs will be compiled and graded on the course GitHub Codespace. Code that does not compile with the provided makefile will result in a 0.

There are coding resources and working code you may use in the course GitHub repositories.  You are free to use any of that code in your program if needed. You may use no other outside code.

## Academic Integrity
This assignment must be 100% your own work. No code may be copied from friends,  previous students, books, web pages, etc. All code submitted is automatically checked 
against a database of previous semester’s graded assignments, current student’s code and common web sources. By submitting your code on GitHub you are attesting that 
you have neither given nor received unauthorized assistance on this work. Code that is copied from an external source or used as inspiration, excluding the 
course github, will result in a 0 for the assignment and referral to the Office of Student Conduct.

