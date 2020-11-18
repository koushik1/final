#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

int releaseall (int numlocks, long args,...)
{

	STATWORD ps;
	disable(ps);
	struct lentry *lptr;
	struct pentry *pptr;
	
	int i;
	int ld;
    unsigned long *lock; 
	int is_lock_valid = 0;
	pptr = &proctab[currpid];
	
	lock = (unsigned long *)(&args);
	for (i=0;i<numlocks;i++)
	{
		ld = *lock++;

		if ((ld<0 || ld>=NLOCKS)) 
            is_lock_valid = 1;	   	
		else
		{
			lptr = &locks[ld];
			if (lptr->process_bitmap[currpid] == 1)
			{
				int lock_current_held = 1;
				int i=0;

				lptr->process_bitmap[currpid] = 0;
				lptr->ltype = DELETED;
				pptr->lock_bitmap[ld] = 0;
				pptr->lock_id = -1;
				pptr->waiting_on_type = -1;
				int max_priority = max_current_process_priority(currpid);
				
				if (max_priority > pptr->pprio)
					pptr->pinh = max_priority;
				else
					pptr->pinh = 0; 
					
				for (i = 0;i < NPROC;i++)
				{
					if (lptr->process_bitmap[i] == 1)
					{
						lock_current_held = 0;
						break;
					}
				}

				if (lock_current_held)
				{
					if (nonempty(lptr->q_head))
					{
						int prev = lptr->q_tail;
						int writer_flag = 0;
						int wpid = 0;

						struct pentry *temp_pointer;
						struct pentry *max_write_pointer;
						
						while (q[prev].qprev != lptr->q_head)
						{
							prev = q[prev].qprev;
							max_write_pointer = &proctab[prev];
							if (max_write_pointer->waiting_on_type == WRITE)
							{
								writer_flag = 1;
								wpid = prev;
								break;		
							}	
						}
						

						if (writer_flag)
						{
							prev = lptr->q_tail;
							if (q[wpid].qkey == q[q[lptr->q_tail].qprev].qkey)
							{
								unsigned long time_dif = 0;	
								if (proctab[q[prev].qprev].wait_time > max_write_pointer->wait_time)
									time_dif = proctab[q[prev].qprev].wait_time - max_write_pointer->wait_time;
								else
									time_dif =  max_write_pointer->wait_time - proctab[q[prev].qprev].wait_time;
								
								if (time_dif < 1000) 
								{
										dequeue(wpid);
										temp_pointer = &proctab[wpid];
										assign_lock(lptr,temp_pointer, wpid, WRITE,ld);
										ready(wpid, RESCHNO);
								}
								else
								{
									prev = lptr->q_tail;
									while (q[prev].qprev != wpid)
									{
										prev = q[prev].qprev;
										dequeue(prev);
										temp_pointer = &proctab[prev];
										assign_lock(lptr,temp_pointer, prev, READ,ld);
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
									temp_pointer = &proctab[prev];
									assign_lock(lptr,temp_pointer, prev, READ,ld);
									ready(prev, RESCHNO);
								}
							}
						}
						else
						{
							prev = lptr->q_tail;
							
							while (q[prev].qprev != lptr->q_head && q[prev].qprev < NPROC && q[prev].qprev > 0)
							{	
								prev = q[prev].qprev;
								dequeue(prev);
								temp_pointer = &proctab[prev];
								assign_lock(lptr,temp_pointer, prev, READ,ld);
								ready(prev, RESCHNO);
							}	
						}
								
					}
				}
					
				lptr->lprio = max_waiting_process_priority(ld);			
			}

			else
			{
				is_lock_valid = 1;
			} 
		}		
	}

	resched();

	restore(ps);
	if(is_lock_valid)
		return SYSERR;
	return OK;
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