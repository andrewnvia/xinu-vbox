/* fork.c - fork */

#include <xinu.h>

local	int newpid();

/*------------------------------------------------------------------------
 *  fork  -  Create forked process
 *------------------------------------------------------------------------
 */
pid32	fork(void)
{
    void		*funcaddr = INITRET;	/* Address of the function	*/
    uint32	ssize = 8192;		/* Stack size in bytes		*/
    pri16		priority = 50;	/* Process priority > 0		*/
    char		name[PNMLEN]= "copy attempt";		/* Name (for debugging)		*/
    unsigned long ebx, esi, edi;

	asm("movl %%ebx, %0\n" :"=r"(ebx));
	asm("movl 4(%%ebp), %0\n" :"=r"(funcaddr));
	asm("movl %%esi, %0\n" :"=r"(esi));
	asm("movl %%edi, %0\n" :"=r"(edi));

	uint32		savsp, *pushsp;
	intmask 	mask;    	/* Interrupt mask		*/
	pid32		pid;		/* Stores new process id	*/
	struct	procent	*prptr;		/* Pointer to proc. table entry */
	int32		i;
	uint32		*saddr;		/* Stack address		*/

	mask = disable();
    
	if (ssize < MINSTK)
		ssize = MINSTK;
	ssize = (uint32) roundmb(ssize);
	if ( (priority < 1) || ((pid=newpid()) == SYSERR) ||
	     ((saddr = (uint32 *)getstk(ssize)) == (uint32 *)SYSERR) ) {
		restore(mask);
		return SYSERR;
	}

	prcount++;
	prptr = &proctab[pid];

	/* Initialize process table entry for new process */
	prptr->prstate = PR_SUSP;	/* Initial state is suspended	*/
	prptr->prprio = priority;
	prptr->prstkbase = (char *)saddr;
	prptr->prstklen = ssize;
	prptr->prname[PNMLEN-1] = NULLCH;
	for (i=0 ; i<PNMLEN-1 && (prptr->prname[i]=name[i])!=NULLCH; i++)
		;
	prptr->prsem = -1;
	prptr->prparent = (pid32)getpid();
	prptr->prhasmsg = FALSE;
	prptr->user_process = TRUE;

	/* Set up stdin, stdout, and stderr descriptors for the shell	*/
	prptr->prdesc[0] = CONSOLE;
	prptr->prdesc[1] = CONSOLE;
	prptr->prdesc[2] = CONSOLE;

	/* Initialize stack as if the process was called		*/

	*saddr = STACKMAGIC;
	savsp = (uint32)saddr;
    kprintf("%X\n",funcaddr);
    kprintf("%X\n",prptr->prstkbase);

	/* Recreating the stack of process 5 in the main.fork testbench */
    *--saddr = 0;
    *--saddr = (long)INITRET;
    *--saddr = savsp;
    *--saddr = 0;
    *--saddr = 0;
    *--saddr = 0;
    *--saddr = 0;
    *--saddr = 0;
    *--saddr = 0;
    *--saddr = (long) funcaddr;
    *--saddr = savsp - 12;
	*--saddr = mask;	/* Copy interrupts */

	/* Basically, the following emulates an x86 "pushal" instruction*/

	*--saddr = NPROC;			/* %eax */
	*--saddr = 0;			/* %ecx */
	*--saddr = 0;			/* %edx */
	*--saddr = ebx;			/* %ebx */
	*--saddr = 0;			/* %esp; value filled in below	*/
	pushsp = saddr;			/* Remember this location	*/
	*--saddr = savsp;		/* %ebp (while finishing ctxsw)	*/
	*--saddr = 0;			/* %esi */
	*--saddr = 0;			/* %edi */
	*pushsp = (unsigned long) (prptr->prstkptr = (char *)saddr);
    resched_cntl(DEFER_START);
    resume(6);
    resched_cntl(DEFER_STOP);
	restore(mask);
	return pid;
}

/*------------------------------------------------------------------------
 *  newpid  -  Obtain a new (free) process ID
 *------------------------------------------------------------------------
 */
local	pid32	newpid(void)
{
	uint32	i;			/* Iterate through all processes*/
	static	pid32 nextpid = 1;	/* Position in table to try or	*/
					/*   one beyond end of table	*/

	/* Check all NPROC slots */

	for (i = 0; i < NPROC; i++) {
		nextpid %= NPROC;	/* Wrap around to beginning */
		if (proctab[nextpid].prstate == PR_FREE) {
			return nextpid++;
		} else {
			nextpid++;
		}
	}
	return (pid32) SYSERR;
}
