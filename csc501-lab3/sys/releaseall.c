#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

int releaseall (int numlocks, long lks,...)
{

	STATWORD ps;
	struct lentry *lptr;
	struct pentry *pptr;
	disable(ps);
	
	int i;
	int ld;
        unsigned long *a; 
	int flag = 0;
	pptr = &proctab[currpid];
	
	a = (unsigned long *)(&lks);
	for (i=0;i<numlocks;i++)
	{
		ld = *a++;

		if ((ld<0 || ld>=NLOCKS)) 
		{
               		flag = 1;	   	
       		}
		else
		{
			lptr = &locks[ld];
			if (lptr->process_bitmap[currpid] == 1)
			{
				releaseLDForProc(currpid, ld);					
			}
			else
			{
				flag = 1;
			} 
		}		
	}

	resched();

	restore(ps);
	return flag == 0 ? OK : SYSERR;	
}

void releaseLDForProc(int pid, int ld)
{
	
	struct lentry *lptr;
	struct pentry *pptr;
	struct pentry *nptr;
	struct pentry *wptr;
	
	int oltype = lptr->ltype;
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
				nptr->lock_bitmap[ld] = 1;

 				lptr->ltype = READ;
				lptr->process_bitmap[prev] = 1;
				nptr->wait_time = 0;
				nptr->lock_id = -1;
				nptr->waiting_on_type = -1;

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
						nptr->lock_bitmap[ld] = 1;
	
 						lptr->ltype = WRITE;
						lptr->process_bitmap[wpid] = 1;
						nptr->wait_time = 0;
						nptr->lock_id = -1;
						nptr->waiting_on_type = -1;

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
						nptr->lock_bitmap[ld] = 1;

 						lptr->ltype = READ;
						lptr->process_bitmap[prev] = 1;
						nptr->wait_time = 0;
						nptr->lock_id = -1;
						nptr->waiting_on_type = -1;

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
					nptr->lock_bitmap[ld] = 1;

 					lptr->ltype = READ;
					lptr->process_bitmap[prev] = 1;
					nptr->wait_time = 0;
					nptr->lock_id = -1;
					nptr->waiting_on_type = -1;

					ready(prev, RESCHNO);
				}
			}
		}
				
	}
		
	lptr->lprio = max_waiting_process_priority(ld);
	maxprio = max_current_process_priority(pid);
	
	if (maxprio > pptr->pprio)
	{
		pptr->pinh = maxprio;
	}
	else
	{
		pptr->pinh = 0; 
	}
	
			
}

