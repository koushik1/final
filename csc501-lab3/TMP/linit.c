#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

struct  lentry  locks[NLOCKS];
int 	lock_index;		

void linit()
{

    struct lentry *lptr;
    int i=0;
    lock_index = 0;

    while(i < NLOCKS){

        lptr = &locks[i];
        lptr->lstate = LFREE;
        lptr->q_head = newqueue();
        lptr->q_tail = 1 + lptr->q_head;
	    lptr->lprio  = -1;	
		lptr->ltype = DELETED;
        int j=0;
        while(j<NPROC)
        {
            lptr->process_bitmap[j] = 0;
            j++;
        }
        i++;
    }
}
