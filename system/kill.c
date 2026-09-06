/* kill.c - kill */

#include <xinu.h>

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
	struct	procent *childptr;		/* Ptr to process's table entry	*/
	struct	procent *innerchildptr;		/* Ptr to process's table entry	*/
	int32	i;			/* Index for loops	*/
	int32   j;			/* Index for inner current loop */
	static int32 killcounter = 0;
	static bool8 currentkill = FALSE;
	mask = disable();
	if (isbadpid(pid) || (pid == NULLPROC)
	    || ((prptr = &proctab[pid])->prstate) == PR_FREE) {
		restore(mask);
		return SYSERR;
	}

	if (--prcount <= 1) {		/* Last user process completes	*/
		xdone();
	}

	killcounter++;
	if (prptr->user_process) {
		for (i = 0; i < NPROC; i++) {
			childptr = &proctab[i];
			if (childptr->prstate == PR_FREE 
				|| childptr->prparent != pid || !childptr->user_process) {  /* skip unused slots	*/
				continue;
			}
			if (i != currpid) {
				kill(i);
				continue;
			}
			currentkill = TRUE;
			for (j = 0; j < NPROC; j++) {
				innerchildptr = &proctab[j];
				if (innerchildptr->prstate == PR_FREE 
					|| innerchildptr->prparent != currpid || !innerchildptr->user_process) {  /* skip unused slots	*/
					continue;
				}
				kill(j);
			}
		}
	}
	killcounter--;

	send(prptr->prparent, pid);
	for (i=0; i<3; i++) {
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

	/* Kill current after cascade */
	if (currentkill && killcounter == 0) {
		currentkill = FALSE;
		kill(currpid);
	}

	restore(mask);
	return OK;
}
