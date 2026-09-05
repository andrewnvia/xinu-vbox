/* xsh_spawnkill.c - xsh_spawnkill */

#include <xinu.h>
#include <string.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 * xsh_spawnkill - message a spawned process, which then kills the second given pid
 *------------------------------------------------------------------------
 */
shellcmd xsh_spawnkill(int nargs, char *args[]) {

	pid32	wakepid;			/* ID of process to wake	*/
	pid32	killpid;			/* ID of process to kill	*/
	char	ch;			/* next character of argument	*/
	char	*chptr;			/* walks along argument string	*/

	/* Output info for '--help' argument */

	if (nargs == 2 && strncmp(args[1], "--help", 7) == 0) {
		printf("Usage: %s WakePID KillPID\n\n", args[0]);
		printf("Description:\n");
		printf("\tWakes process WakePID spawned by the spawn command, which then kills process KillPID\n");
		printf("Options:\n");
		printf("\tWakePID \tthe ID of a process that will begin running\n");
		printf("\tKillPID \tthe ID of a process to terminate\n");
		printf("\t--help\tdisplay this help and exit\n");
		return OK;
	}

	/* Check argument count */

	if (nargs != 3) {
		fprintf(stderr, "%s: incorrect argument\n", args[0]);
		fprintf(stderr, "Try '%s --help' for more information\n",
			args[0]);
		return SYSERR;
	}

	/* compute process ID from argument string */

	chptr = args[1];
	ch = *chptr++;
	wakepid = 0;
	while(ch != NULLCH) {
		if ( (ch < '0') || (ch > '9') ) {
			fprintf(stderr, "%s: non-digit in process ID\n",
				args[0]);
			return 1;
		}
		wakepid = 10*wakepid + (ch - '0');
		ch = *chptr++;
	}
	if (wakepid == 0) {
		fprintf(stderr, "%s: cannot wake the null process\n",
			args[0]);
		return 1;
	}

	chptr = args[2];
	ch = *chptr++;
	killpid = 0;
	while(ch != NULLCH) {
		if ( (ch < '0') || (ch > '9') ) {
			fprintf(stderr, "%s: non-digit in process ID\n",
				args[0]);
			return 1;
		}
		killpid = 10*killpid + (ch - '0');
		ch = *chptr++;
	}
	if (killpid == 0) {
		fprintf(stderr, "%s: cannot kill the null process\n",
			args[0]);
		return 1;
	}
	send(wakepid, killpid);
	ready(wakepid);

	return 0;
}
