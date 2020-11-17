#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

int releaseall (int numlocks, long args,...)
{

	STATWORD ps;
	struct lentry *lptr;
	struct pentry *pptr;
	disable(ps);
	
	int ld;
    unsigned long *locks; 
	int error = 0;
	pptr = &proctab[currpid];
	
	locks = (unsigned long *)(&args);
	int i;
	for (i=0;i<numlocks;i++)
	{
		ld = *locks++;

		if ((ld<0 || ld>=NLOCKS)) 
		{
            restore(ps);
			return SYSERR;	
       	}

		lptr = &locks[ld];
		if (lptr->process_bitmap[currpid] == 1)
		{
			releaseLDForProc(currpid, ld);					
		}
		else
		{
			error = 1;
		}
		
	}


	restore(ps);
	resched();
	if (!error)
		return OK;
	return SYSERR;	
}

void releaseLDForProc(int pid, int ld)
{
	
	struct lentry *lptr;
	struct pentry *pptr;
	struct pentry *nptr;
	struct pentry *wptr;
	
	int maxprio = -1;
	int i=0;

	lptr = &locks[ld];
	pptr = &proctab[pid];

	lptr->ltype = DELETED;
	lptr->process_bitmap[pid] = 0;
	
	pptr->lock_bitmap[ld] = 0;
	pptr->lock_id = -1;
	pptr->waiting_on_type = -1;



	if (nonempty(lptr->q_head))
	{
		int prev = lptr->q_tail;
		int writerProcExist = 0;
		int readerProcHoldingLock = 0;
		int wpid = 0;
		struct qent *mptr;
		unsigned long tdf = 0;	
		maxprio = q[q[prev].qprev].qkey;
		
		while (q[prev].qprev != lptr->q_head)
		{
			prev = q[prev].qprev;
			wptr = &proctab[prev];
			if (wptr->waiting_on_type == WRITE)
			{
				writerProcExist = 1;
				wpid = prev;
				mptr = &q[wpid];
				break;		
			}	
		}
		
		if (writerProcExist == 0)
		{
			prev = lptr->q_tail;
			
			while (q[prev].qprev != lptr->q_head && q[prev].qprev < NPROC && q[prev].qprev > 0)
			{	
				prev = q[prev].qprev;
				dequeue(prev);
				nptr = &proctab[prev];
				assign_lock(lptr,nptr, prev, READ,ld);
				ready(prev, RESCHNO);
			}	
		}
		else if (writerProcExist == 1)
		{
			prev = lptr->q_tail;
			if (mptr->qkey == maxprio)
			{
				tdf = proctab[q[prev].qprev].wait_time - wptr->wait_time;
				if (tdf < 0)
				{
					tdf = (-1)*tdf; 
				}
				if (tdf < 1000) 
				{
					for (i = 0;i < NPROC;i++)
					{
						if (lptr->process_bitmap[i] == 1)
						{
							readerProcHoldingLock = 1;
							break;
						}
					}
					if (readerProcHoldingLock == 0)
					{
					
						dequeue(wpid);
	
						nptr = &proctab[wpid];
				        assign_lock(lptr,nptr, wpid, WRITE,ld);
						ready(wpid, RESCHNO);
					}
				}
				else
				{
					prev = lptr->q_tail;
					while (q[prev].qprev != wpid)
					{
						prev = q[prev].qprev;
						dequeue(prev);
						nptr = &proctab[prev];
				        assign_lock(lptr,nptr, prev, READ,ld);
						ready(prev, RESCHNO);
					}				
				}
			}
			else
			{
				prev = lptr->q_tail;
				while (q[prev].qprev != wpid)
				{
					prev = q[prev].qprev;
					dequeue(prev);
					nptr = &proctab[prev];
			        assign_lock(lptr,nptr, prev, READ,ld);
					ready(prev, RESCHNO);
				}
			}
		}
				
	}
	lptr->lprio = max_waiting_process_priority(ld);
	maxprio = max_current_process_priority(pid);
	
	if (maxprio > pptr->pprio)
		pptr->pinh = maxprio;
	else
		pptr->pinh = 0; 
		
				
}

void assign_lock(struct lentry *lptr,struct pentry *pptr, int pid, int type,int ld)
{
	pptr->lock_bitmap[ld] = 1;
	pptr->wait_time = 0;
	pptr->lock_id = -1;
	pptr->waiting_on_type = -1;
	lptr->ltype = type;
	lptr->process_bitmap[pid] = 1;
}
