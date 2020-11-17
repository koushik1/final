
#ifndef _LOCK_H_
#define _LOCK_H_

#ifndef NLOCKS
#define	NLOCKS		50	
#endif


#ifndef DELETED
#define DELETED -6
#endif

#define READ 0
#define WRITE 1

#define	LFREE 1	
#define	LUSED 2	

struct	lentry	{		
	char lstate;			
	int	ltype;		
	int	lprio;		
	int	q_head;		
	int	q_tail;	
	int process_bitmap[NPROC]; 
};
extern	struct	lentry	locks[];
extern unsigned long ctr1000;
extern	int	lock_index;

#endif
