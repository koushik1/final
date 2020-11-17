#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

int lock (int ldes1, int type, int priority)
{
	STATWORD ps;
	struct lentry *lptr;
	struct pentry *pptr;
	disable(ps);
	
	if ((ldes1<0 || ldes1>=NLOCKS) || (lptr= &locks[ldes1])->lstate==LFREE) {
		restore(ps);
		return(SYSERR);
	}
	
	pptr = &proctab[currpid];

	if (lptr->ltype == DELETED)
	{
		lptr->ltype = type;
		lptr->lprio = -1;
		lptr->lproc_list[currpid] = 1;

		pptr->bm_locks[ldes1] = 1; 
		pptr->wait_lockid = -1; 
		pptr->wait_pprio = priority; 
		pptr->wait_ltype = -1; 	
	}
	
	else if (lptr->ltype == READ)
	{
		if (type == WRITE)
		{
			restore(ps);
			return block_process(pptr,ldes1,priority,type,currpid);
		}		
				
		else if (type == READ)
		{
			int writerProcExist = 0;
			int next = lptr->lqhead;
			struct pentry *wptr;
			
			while (q[next].qnext != lptr->lqtail)
			{
				next = q[next].qnext;
				wptr = &proctab[next];
				if (wptr->wait_ltype == WRITE && q[next].qkey > priority)
				{
					writerProcExist = 1;
					break;	
				}	
			}
			
			if (writerProcExist == 0)
			{
				lptr->ltype = type;
				lptr->lprio = getMaxPriorityInLockWQ(ldes1); 
				lptr->lproc_list[currpid] = 1;

				pptr->bm_locks[ldes1] = 1;
				pptr->wait_lockid = -1; 
				pptr->wait_pprio = priority; 
				pptr->wait_ltype = -1; 
				rampUpProcPriority (ldes1,-1);	
			}
			
			if (writerProcExist == 1)
			{
				restore(ps);
				return block_process(pptr,ldes1,priority,type,currpid);
			}
		}			
	}

	else if (lptr->ltype == WRITE)
	{
		restore(ps);
		return block_process(pptr,ldes1,priority,type,currpid);
		
	}
	
	restore(ps);
	return(OK); 
}

int getMaxPriorityInLockWQ (int ld)
{
	struct lentry *lptr;
	struct pentry *pptr;

	int lprio = -1;
	int gprio = -1;	
	lptr = &locks[ld];
	
	int next = lptr->lqhead;
	while (q[next].qnext != lptr->lqtail)
	{
		next = q[next].qnext;
		pptr = &proctab[next];

		if (pptr->pinh == 0)
			gprio =  pptr->pprio;
		else
			gprio =  pptr->pinh;


		if (lprio < gprio)
		{
			lprio = gprio;
		} 		
	}

	return lprio;				
}

int block_process(struct pentry *pptr,int lock_d,int priority,int type,int pid)
{
    struct lentry *lptr;
    lptr= &locks[lock_d];
    pptr->pstate = PRWAIT;
    pptr->wait_lockid = lock_d;  
    pptr->wait_time = ctr1000; 
	pptr->wait_pprio = priority;  
    pptr->wait_ltype = type; 

    insert(pid, lptr->lqhead, priority); 
    lptr->lprio = getMaxPriorityInLockWQ(lock_d); 
    int current_prio = -1;

    if (pptr->pinh == 0)
        current_prio =  pptr->pprio;
    else
        current_prio =  pptr->pinh;

    rampUpProcPriority(lock_d,current_prio); 		 
    resched();
    return OK;	
}

void rampUpProcPriority (int ld, int priority)
{
	struct lentry *lptr;
	struct pentry *pptr;
	int i;
	int tmpld;
	int gprio = -1;
	int maxprio = -1;				
	lptr = &locks[ld];

	for (i=0;i<NPROC;i++)
	{
		if (lptr->lproc_list[i] == 1)
		{
			pptr = &proctab[i];
			if (pptr->pinh == 0)
				gprio =  pptr->pprio;
			else
				gprio =  pptr->pinh;
			
		 	
			if (priority == -1)
			{
				maxprio = getMaxWaitProcPrioForPI(i);
				
				
				if (maxprio > pptr->pprio)
				{
					pptr->pinh = maxprio;
				}
				else
				{
					pptr->pinh = 0; 
				}
				
				tmpld = checkProcessTransitivityForPI(i);
				if (tmpld != -1)
				{
					rampUpProcPriority (tmpld,-1);
				}			 
			}
			
			else if (gprio < priority)
			{
				pptr->pinh = priority;
				tmpld = checkProcessTransitivityForPI(i);
				if (tmpld != -1)
				{
					rampUpProcPriority (tmpld,-1);
				}			 
			}	
		}
	} 		

}

int getMaxWaitProcPrioForPI (int pid)
{
	int i;
	int maxprio = -1;
	struct pentry *pptr;
	struct lentry *lptr;

	pptr = &proctab[pid];
	
	for (i=0;i<NLOCKS;i++)
	{
		if (pptr->bm_locks[i] == 1)
		{
			lptr = &locks[i];
			if (maxprio < lptr->lprio)
			{
				maxprio = lptr->lprio;
			}
		}		
	}
		
	return maxprio;
}

int checkProcessTransitivityForPI (int pid)
{
	int ld;
	struct pentry *pptr;

	pptr = &proctab[pid];
	ld = pptr->wait_lockid;
	if ((ld<0 || ld>=NLOCKS))
	{
		return -1;
	}
	else
	{
		return ld;
	}			
}	
