#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void run_test() 
{
    int pid = fork();
    if (pid == 0) 
    {
        setPriority(getpid(), -20);  // High priority
        while (1);
    }

    pid = fork();
    if (pid == 0) 
    {
        setPriority(getpid(), 0);  // Medium priority
        while (1);
    }

    pid = fork();
    if (pid == 0) 
    {
        setPriority(getpid(), 20);  // Low priority
        while (1);
    }

    while (wait(0) > 0);
}

int main(void) 
{
    run_test();
    exit(0);
}
