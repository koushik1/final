/* chprio.c - chprio */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <stdio.h>
#include <lock.h>

/*------------------------------------------------------------------------
 * chprio  --  change the scheduling priority of a process
 *------------------------------------------------------------------------
 */
SYSCALL chprio(int pid, int newprio)
{
	STATWORD ps;    
	struct	pentry	*pptr;
	struct lentry *lptr;
	int ld;
	disable(ps);
	if (isbadpid(pid) || newprio<=0 ||
	    (pptr = &proctab[pid])->pstate == PRFREE) {
		restore(ps);
		return(SYSERR);
	}
	if (newprio > pptr->pprio)
		pptr->pinh = newprio;
	else
	{
		pptr->pprio = newprio;
		pptr->pinh = 0;
	}

	ld = pptr->lock_id;
	lptr = &locks[ld];
	lptr->lprio = max_waiting_process_priority(ld);
	update_process_priority(ld,-1);	
	restore(ps);
	return(newprio);
}
