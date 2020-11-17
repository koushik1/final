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

	int	ld;
    int i;
    for (i = 0; i < NLOCKS; i++)
    {
        if (lock_index == NLOCKS)
        {
            lock_index = 0;
        }
        ld = lock_index;
        lock_index = lock_index + 1;
		if (locks[ld].lstate==LFREE) {
			locks[ld].lstate = LUSED;
            restore(ps);
			return(ld);
		}

    }
    restore(ps);
    return(SYSERR);
}


