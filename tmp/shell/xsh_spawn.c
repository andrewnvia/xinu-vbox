/* xsh_spawn.c - xsh_spawn */

#include <xinu.h>
#include <string.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 * xsh_spawn - create a process tree of depth x, each awaiting a message
 *------------------------------------------------------------------------
 */
shellcmd xsh_spawn(int nargs, char *args[]) {

	pid32	depth;			/* ID of process to kill	*/
	char	ch;			/* next character of argument	*/
	char	*chptr;			/* walks along argument string	*/

	/* Output info for '--help' argument */

	if (nargs == 2 && strncmp(args[1], "--help", 7) == 0) {
		printf("Usage: %s Depth\n\n", args[0]);
		printf("Description:\n");
		printf("\tCreates a binary tree of processes Depth deep. Each one can be awoken with a message containing a process to kill.\n");
		printf("Options:\n");
		printf("\tDepth \tThe depth of the tree to create\n");
		printf("\t--help\tdisplay this help and exit\n");
		return OK;
	}

	/* Check argument count */

	if (nargs != 2) {
		fprintf(stderr, "%s: incorrect argument\n", args[0]);
		fprintf(stderr, "Try '%s --help' for more information\n",
			args[0]);
		return SYSERR;
	}

	/* compute process ID from argument string */

	chptr = args[1];
	ch = *chptr++;
	depth = 0;
	while(ch != NULLCH) {
		if ( (ch < '0') || (ch > '9') ) {
			fprintf(stderr, "%s: non-digit in Depth\n",
				args[0]);
			return 1;
		}
		depth = 10*depth + (ch - '0');
		ch = *chptr++;
	}
	if (depth == 0) {
		fprintf(stderr, "%s: cannot create tree of depth 0\n",
			args[0]);
		return 1;
	}

    resume(create(spawn, 512, 150, "spawn root", 1, depth));
	send(proctab[currpid].prparent, currpid);
	suspend(currpid);
	return 0;
}
