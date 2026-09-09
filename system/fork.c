/* fork.c - fork */

#include <xinu.h>

/*------------------------------------------------------------------------
 *  fork  -  Create forked process nearly identical to current
 *------------------------------------------------------------------------
 */
pid32	fork(void)
{
    /* Values gathered from original process    */
    unsigned long ebx, ebp, esi, edi;
    void		*funcaddr;	/* Address of the function	*/
    uint32	ssize;		/* Stack size in bytes		*/
    pri16		priority;	/* Process priority > 0		*/
    char		*name;		/* Name (for debugging)		*/

    /* Values created for new process   */
	uint32		*pushsp;
	intmask 	mask;    	/* Interrupt mask		*/
	pid32		pid;		/* Stores new process id	*/
	struct	procent	*prptr, *oldptr;		/* Pointer to proc. table entry */
	int32		i;
	uint32		*saddr;		/* Stack address		*/

    /* Values used for constructing stack   */
    uint32 baseoffset, stackoffset;
	unsigned long	*sp, *fp, *copyptr;

	mask = disable();

    oldptr = &proctab[getpid()];

	asm("movl %%ebx, %0\n" :"=r"(ebx));
	asm("movl %%ebp, %0\n" :"=r"(ebp));
	asm("movl %%esi, %0\n" :"=r"(esi));
	asm("movl %%edi, %0\n" :"=r"(edi));
	asm("movl 4(%%ebp), %0\n" :"=r"(funcaddr));
    ssize = oldptr->prstklen;
    priority = oldptr->prprio;
    name = oldptr->prname;

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

    /* Create base of stack */
    *saddr = STACKMAGIC;

    /* Copy stack with modified FPs */
    sp = (unsigned long *)ebp;
    fp = (unsigned long *)ebp;
    baseoffset = prptr->prstkbase - oldptr->prstkbase;
    stackoffset =  (uint32 *)sp - (uint32 *)oldptr->prstkbase;
    copyptr = (unsigned long *)(saddr + stackoffset);
    saddr = (uint32 *)copyptr;

    while (sp < (unsigned long *)oldptr->prstkbase) {
        for (; sp < fp; sp++){
            *copyptr = *sp;
            copyptr++;
        }
		if (*sp == STACKMAGIC)
			break;
        *copyptr = *sp + baseoffset;
		fp = (unsigned long *) *sp++;
        copyptr++;
		if (fp <= sp) {
			kprintf("bad stack, fp (%08X) <= sp (%08X)\n", fp, sp);
			return SYSERR;
		}
	}
	/* Recreating the stack of process 5 in the main.fork testbench */
    
	*--saddr = mask;	/* Copy interrupts */

	/* Basically, the following emulates an x86 "pushal" instruction*/

	*--saddr = NPROC;		/* %eax */
	*--saddr = 0;			/* %ecx */
	*--saddr = 0;			/* %edx */
	*--saddr = ebx;			/* %ebx */
	*--saddr = 0;			/* %esp; value filled in below	*/
	pushsp = saddr;			/* Remember this location	*/
	*--saddr = ebp + baseoffset;		/* %ebp (while finishing ctxsw)	*/
	*--saddr = esi;			/* %esi */
	*--saddr = edi;			/* %edi */
	*pushsp = (unsigned long) (prptr->prstkptr = (char *)saddr);
	prptr->prstate = PR_READY;
	insert(pid, readylist, prptr->prprio);
	restore(mask);
	return pid;
}
