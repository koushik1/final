#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

LOCAL int newlock();

int lcreate()
{
	STATWORD ps;    
	disable(ps);

	int	lock_desc;
    int i;
    for (i = 0; i < NLOCKS; i++)
    {
        if (lock_index == NLOCKS)
        {
            lock_index = 0;
        }
        lock_desc = lock_index;
        lock_index = lock_index + 1;
		if (locks[lock_desc].lstate==LFREE) {
			locks[lock_desc].lstate = LUSED;
            restore(ps);
			return(lock_desc);
		}

    }
    restore(ps);
    return(SYSERR);
}


