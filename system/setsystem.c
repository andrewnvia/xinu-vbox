/* setsystem.c - setsystem */

#include <xinu.h>

/*------------------------------------------------------------------------
 *  setsystem  -  Set given process label to system process
 *------------------------------------------------------------------------
 */
void	setsystem(pid32 pid)
{
	struct	procent *prptr;		/* Ptr to process's table entry	*/
	prptr = &proctab[pid];
	prptr->user_process = FALSE;
}
