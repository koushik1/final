/* kill.c - kill */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <q.h>
#include <stdio.h>
#include <lock.h>
/*------------------------------------------------------------------------
 * kill  --  kill a process and remove it from the system
 *------------------------------------------------------------------------
 */
SYSCALL kill(int pid)
{
	STATWORD ps;    
	struct	pentry	*pptr;		/* points to proc. table for pid*/
	int	dev;
	int ld;
	disable(ps);
	if (isbadpid(pid) || (pptr= &proctab[pid])->pstate==PRFREE) {
		restore(ps);
		return(SYSERR);
	}
	if (--numproc == 0)
		xdone();

	dev = pptr->pdevs[0];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->pdevs[1];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->ppagedev;
	if (! isbaddev(dev) )
		close(dev);
	
	send(pptr->pnxtkin, pid);

	freestk(pptr->pbase, pptr->pstklen);

	for (ld = 0; ld < NLOCKS; ld++)
	{		if (pptr->lock_bitmap[ld] == 1) 
		{
			struct lentry *temp_lptr;
			
			temp_lptr = &locks[ld];
			pptr = &proctab[pid];

			dequeue(pid);
			pptr->lock_id = -1;
			pptr->waiting_on_type = -1;
			pptr->wait_time = 0;

			temp_lptr->lprio = max_waiting_process_priority(ld);	
			update_process_priority(ld,-1);
		}
	}
		
	switch (pptr->pstate) {

	case PRCURR:	pptr->pstate = PRFREE;	/* suicide */
			resched();

	case PRWAIT:	semaph[pptr->psem].semcnt++;
					ld = pptr->lock_id;
					if (ld>=0 || ld<NLOCKS)
					{
						pptr->pinh = 0;
						struct lentry *temp_lptr;
						
						temp_lptr = &locks[ld];
						pptr = &proctab[pid];

						dequeue(pid);
						pptr->lock_id = -1;
						pptr->waiting_on_type = -1;
						pptr->wait_time = 0;

						temp_lptr->lprio = max_waiting_process_priority(ld);	
						update_process_priority(ld,-1);
					}

	case PRREADY:	dequeue(pid);
			pptr->pstate = PRFREE;
			break;

	case PRSLEEP:
	case PRTRECV:	unsleep(pid);
						/* fall through	*/
	default:	pptr->pstate = PRFREE;
	}
	
	restore(ps);
	return(OK);
}
