#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

int lock (int ld, int type, int priority)
{
	STATWORD ps;
	disable(ps);
	struct lentry *lptr;
	struct pentry *pptr;
	lptr= &locks[ld];
	pptr = &proctab[currpid];

	if ((ld<0 || ld>=NLOCKS)) 
	{
		restore(ps);
		return(SYSERR);
	}

	if (lptr->lstate==LFREE) 
	{
		restore(ps);
		return(SYSERR);
	}
	

	
	if (lptr->ltype == READ)
	{
		if (type == WRITE)
		{
			restore(ps);
			return block_process(pptr,ld,priority,type,currpid);
		}		
				
		else if (type == READ)
		{
			if (check_higher_priority_writer(ld,priority))
			{
				restore(ps);
				return block_process(pptr,ld,priority,type,currpid);
			}
			
			else

			{
				lptr->ltype = type;
				lptr->lprio = getMaxPriorityInLockWQ(ld); 
				lptr->lproc_list[currpid] = 1;

				pptr->bm_locks[ld] = 1;
				pptr->wait_lockid = -1; 
				pptr->wait_pprio = priority; 
				pptr->wait_ltype = -1; 
				rampUpProcPriority (ld,-1);	
			}
			

		}			
	}

	else if (lptr->ltype == WRITE)
	{
		restore(ps);
		return block_process(pptr,ld,priority,type,currpid);
		
	}

	else
	{
		lptr->ltype = type;
		lptr->lprio = -1;
		lptr->lproc_list[currpid] = 1;

		pptr->bm_locks[ld] = 1; 
		pptr->wait_lockid = -1; 
		pptr->wait_pprio = priority; 
		pptr->wait_ltype = -1; 	
	}
	
	restore(ps);
	return(OK); 
}

int check_higher_priority_writer(int ld, int priority)
{
	struct lentry *lptr;
	struct pentry *pptr;
	lptr = &locks[ld];
	int curr;
    for(curr = q[lptr->lqhead].qnext; curr!=lptr->lqtail; curr = q[curr].qnext)
    {
		pptr = &proctab[curr];
		if (pptr->wait_ltype == WRITE && q[curr].qkey > priority)
		{
			return 1;
		}	
	}
	return 0;

}

int getMaxPriorityInLockWQ (int ld)
{
    struct lentry *lptr;
    lptr = &locks[ld];
    struct pentry *pptr ;

    int curr ;
    int max_priority = -1;
    for(curr = q[lptr->lqhead].qnext; curr!=lptr->lqtail; curr = q[curr].qnext)
    {
        pptr = &proctab[curr];
        int current_prio = -1;
        if (pptr->pinh == 0)
        {
            current_prio =  pptr->pprio;
        }
        else
        {
            current_prio =  pptr->pinh;
        }

        if (current_prio > max_priority)
            max_priority = current_prio;
    }
    return max_priority;			
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
	lptr = &locks[ld];

	for (i=0;i<NPROC;i++)
	{
		if (lptr->lproc_list[i] == 1)
		{
			pptr = &proctab[i];
			int current_prio = -1;
			if (pptr->pinh == 0)
				current_prio =  pptr->pprio;
			else
				current_prio =  pptr->pinh;

			if (priority == -1)
			{
				int max_p = getMaxWaitProcPrioForPI(i);
				
				if (max_p > pptr->pprio)
					pptr->pinh = max_p;
				else
					pptr->pinh = 0; 
				
				tmpld = proctab[i].wait_lockid;
				if ((tmpld >= 0 && tmpld < NLOCKS))
				{
					rampUpProcPriority (tmpld,-1);
				}			 
			}
			
			else if (current_prio < priority)
			{
				pptr->pinh = priority;
				tmpld = proctab[i].wait_lockid;
				if ((tmpld >= 0 && tmpld < NLOCKS))
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
