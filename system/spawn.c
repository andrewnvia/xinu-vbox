/* spawn.c - spawn */

#include <xinu.h>

/*------------------------------------------------------------------------
 *  spawn  -  Create a tree of processes depth deep
 *------------------------------------------------------------------------
 */
process spawn(
        uint32  depth
    )
{
    pid32       killpid;
    pid32       process1;
    pid32       process2;
    if (depth > 1) {
        chprio(currpid, getprio(currpid) + 1);
        process1 = create(spawn, 512, 150, "spawn child", 1, depth-1);
        process2 = create(spawn, 512, 150, "spawn child", 1, depth-1);
        resume(process1);
        resume(process2);
        chprio(currpid, getprio(currpid) - 1);
    }
    while (TRUE) {
        suspend(currpid);
        killpid = (pid32)receive();
        kill(killpid);
    }
    return OK;
}