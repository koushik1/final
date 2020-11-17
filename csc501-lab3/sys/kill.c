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
	struct lentry *lptr;
	int reschflag = 0;
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
	{
		if (pptr->bm_locks[ld] == 1) 
		{
			struct lentry *temp_lp;
			struct pentry *temp_pp;
			
			temp_lp = &locks[ld];
			temp_pp = &proctab[pid];

			temp_pp->wait_lockid = -1;
			temp_pp->wait_ltype = -1;
			temp_pp->wait_time = 0;
			temp_pp->plockret = DELETED;

			temp_lp->lprio = getMaxPriorityInLockWQ(ld);	
			rampUpProcPriority(ld,-1);
			dequeue(pid);
			reschflag = 1;
		}
	}
		
	switch (pptr->pstate) {

	case PRCURR:	pptr->pstate = PRFREE;	/* suicide */
			resched();

	case PRWAIT:	semaph[pptr->psem].semcnt++;
			ld = pptr->wait_lockid;
			if (ld>=0 || ld<NLOCKS)
			{
				pptr->pinh = 0;
				releaseLDForWaitProc(pid,ld);
			}

	case PRREADY:	dequeue(pid);
			pptr->pstate = PRFREE;
			break;

	case PRSLEEP:
	case PRTRECV:	unsleep(pid);
						/* fall through	*/
	default:	pptr->pstate = PRFREE;
	}

	if (reschflag == 1)
	{
		resched();
	}	
	
	restore(ps);
	return(OK);
}
