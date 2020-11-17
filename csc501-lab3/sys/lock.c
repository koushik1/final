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
				lptr->lprio = max_waiting_process_priority(ld); 
				lptr->process_bitmap[currpid] = 1;

				pptr->lock_bitmap[ld] = 1;
				pptr->lock_id = -1; 
				pptr->waiting_on_type = -1; 
				update_process_priority (ld,-1);	
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
		lptr->process_bitmap[currpid] = 1;

		pptr->lock_bitmap[ld] = 1; 
		pptr->lock_id = -1; 
		pptr->waiting_on_type = -1; 	
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
    for(curr = q[lptr->q_head].qnext; curr!=lptr->q_tail; curr = q[curr].qnext)
    {
		pptr = &proctab[curr];
		if (pptr->waiting_on_type == WRITE && q[curr].qkey > priority)
		{
			return 1;
		}	
	}
	return 0;

}

int max_waiting_process_priority (int ld)
{
    struct lentry *lptr;
    lptr = &locks[ld];
    struct pentry *pptr ;

    int curr ;
    int max_priority = -1;
    for(curr = q[lptr->q_head].qnext; curr!=lptr->q_tail; curr = q[curr].qnext)
    {
        pptr = &proctab[curr];
        int current_prio = -1;
        if (pptr->pinh == 0)
            current_prio =  pptr->pprio;
        else
            current_prio =  pptr->pinh;

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
    pptr->lock_id = lock_d;  
    pptr->waiting_on_type = type; 
    pptr->wait_time = ctr1000; 

    insert(pid, lptr->q_head, priority); 
    lptr->lprio = max_waiting_process_priority(lock_d); 
    int current_prio = -1;

    if (pptr->pinh == 0)
        current_prio =  pptr->pprio;
    else
        current_prio =  pptr->pinh;

    update_process_priority(lock_d,current_prio); 		 
    resched();
    return OK;	
}

int max_current_process_priority (int pid)
{
	int i;
	int maxprio = -1;
	struct pentry *pptr;
	struct lentry *lptr;

	pptr = &proctab[pid];
	
	for (i=0;i<NLOCKS;i++)
	{
		if (pptr->lock_bitmap[i] == 1)
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


void update_process_priority (int ld, int priority)
{
	struct lentry *lptr;
	struct pentry *pptr;
	int i;
	lptr = &locks[ld];

	for (i=0;i<NPROC;i++)
	{
		if (lptr->process_bitmap[i] == 1)
		{
			pptr = &proctab[i];
			int current_prio = -1;
			if (pptr->pinh == 0)
				current_prio =  pptr->pprio;
			else
				current_prio =  pptr->pinh;

			if (priority == -1)
			{
				int max_p = max_current_process_priority(i);
				
				if (max_p > pptr->pprio)
					pptr->pinh = max_p;
				else
					pptr->pinh = 0; 
				
				int new_process_ld = proctab[i].lock_id;
				if ((new_process_ld >= 0 && new_process_ld < NLOCKS))
				{
					update_process_priority (new_process_ld,-1);
				}			 
			}
			
			else if (current_prio < priority)
			{
				pptr->pinh = priority;
				int new_process_ld  = proctab[i].lock_id;
				if ((new_process_ld >= 0 && new_process_ld < NLOCKS))
				{
					update_process_priority (new_process_ld,-1);
				}			 
			}	
		}
	} 		

}

