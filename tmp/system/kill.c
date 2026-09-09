/* kill.c - kill */

#include <xinu.h>

local bool8	cascade(pid32, pid32);
local syscall killproc(pid32);
/*------------------------------------------------------------------------
 *  kill  -  Kill a process and remove it from the system
 *------------------------------------------------------------------------
 */
syscall	kill(
	  pid32		pid		/* ID of process to kill	*/
	)
{
	intmask	mask;			/* Saved interrupt mask		*/
	struct	procent *prptr;		/* Ptr to process's table entry	*/
    pid32 current, rootparent;
    bool8 killcurrent;
	mask = disable();
    current = currpid;
	if (isbadpid(pid) || (pid == NULLPROC)
	    || ((prptr = &proctab[pid])->prstate) == PR_FREE) {
		restore(mask);
		return SYSERR;
	}
    rootparent = prptr->prparent;

    if (prptr->user_process) {
        killcurrent = cascade(pid, current);
    }
    if (current != pid) {
        if (killproc(pid) == OK) {
            if (--prcount <= 1) {		/* Last user process completes	*/
                kprintf("MAJOR ERROR! Impossible for this to be last process, current is still running!\n");
                xdone();
            }
        }
    }

	send(rootparent, pid); // ONLY SHOULD HAPPEN FOR ROOT PROCESS AND ROOT NEEDS TO BE ALREADY DEAD IF NOT CURRENTLY RUNNING
    
    if ((killcurrent || current == pid)) {
        if (--prcount <= 1) {		/* Last user process completes	*/
            xdone();
        }
        killproc(current);
    }
	restore(mask);
	return OK;
}

local bool8 cascade(pid32 pid, pid32 current){
    int32 i, j;     /* loop indices */
	struct	procent *childptr, *innerptr;		/* Ptr to process's table entry	*/
    bool8 hascurrent;

    hascurrent = FALSE;
    for (i = 0; i < NPROC; i++) {
        childptr = &proctab[i];
        if (childptr->prstate == PR_FREE 
            || childptr->prparent != pid || !childptr->user_process) {  /* skip unused slots	*/
            continue;
        }
        if (i != current) {
            if (!hascurrent) {
                hascurrent = cascade(i, current);
            } else {
                cascade(i, current);
            }
            if (killproc(i) == OK) {
                if (--prcount <= 1) {		/* Last user process completes	*/
                    kprintf("MAJOR ERROR! Inner cascade process should not be last \n");
                    xdone();
                }
            }
            continue;
        }
        hascurrent = TRUE;
        for (j = 0; j < NPROC; j++) {
            innerptr = &proctab[j];
            if (innerptr->prstate == PR_FREE 
                || innerptr->prparent != current || !innerptr->user_process) {  /* skip unused slots	*/
                continue;
            }
            cascade(j, current);
            if (killproc(j) == OK) {
                if (--prcount <= 1) {		/* Last user process completes	*/
                    kprintf("MAJOR ERROR! Inner cascade process should not be last \n");
                    xdone();
                }
            }
        }
    }
    
    return hascurrent;
}

local syscall killproc(pid32 pid){
	struct	procent *prptr;		/* Ptr to process's table entry	*/
    int32 i;
	if (isbadpid(pid) || (pid == NULLPROC)
	    || ((prptr = &proctab[pid])->prstate) == PR_FREE) {
		return SYSERR;
	}

	for (i=0; i<3; i++) { //HAPPENS FOR ALL KILLED
		close(prptr->prdesc[i]);
	}
	freestk(prptr->prstkbase, prptr->prstklen);

	switch (prptr->prstate) {
	case PR_CURR:
		prptr->prstate = PR_FREE;	/* Suicide */
		resched();

	case PR_SLEEP:
	case PR_RECTIM:
		unsleep(pid);
		prptr->prstate = PR_FREE;
		break;

	case PR_WAIT:
		semtab[prptr->prsem].scount++;
		/* Fall through */

	case PR_READY:
		getitem(pid);		/* Remove from queue */
		/* Fall through */

	default:
		prptr->prstate = PR_FREE;
	}
    return OK;
}
