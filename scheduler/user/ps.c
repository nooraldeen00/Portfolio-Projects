#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "pstat.h"

int main(int argc, char *argv[]) 
{
    struct pstat ps;

    if (getpinfo(&ps) < 0) 
    {
        printf("Error\n");
        exit(1);
    }

    printf("NAME    PID     STATUS      PRIORITY\n");
    for (int i = 0; i < NPROC; i++) 
    {
        if (ps.inuse[i]) 
        {
            printf("%s    %d    %s    %d\n", ps.name[i], ps.pid[i], 
                   (ps.state[i] == sleep) ? "SLEEPING" : "RUNNING",
                   ps.E_priority[i]);
        }
    }

    exit(0);
}
