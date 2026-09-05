/* spawn.c - spawn */

#include <xinu.h>

/*------------------------------------------------------------------------
 *  spawn  -  Create a tree of processes depth deep
 *------------------------------------------------------------------------
 */
process spawn(
        uint32  depth,
        uint32  prio

    )
{
    pid32       killpid;
    pid32       process1;
    pid32       process2;
    if (depth > 1) {
        chprio(currpid, getprio(currpid) + 1);
        process1 = create(spawn, 512, prio, "spawn child", 2, depth-1, prio);
        process2 = create(spawn, 512, prio, "spawn child", 2, depth-1, prio);
        resume(process1);
        resume(process2);
        chprio(currpid, getprio(currpid) - 1);
    }
    killpid = (pid32)receive();
    kill(killpid);
    return OK;
}