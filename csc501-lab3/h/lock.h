
#ifndef _LOCK_H_
#define _LOCK_H_

#ifndef NLOCKS
#define	NLOCKS		50	
#endif

#define	LFREE 1	
#define	LUSED 2	

#ifndef DELETED
#define DELETED -6
#endif

#define READ 0
#define WRITE 1

struct	lentry	{		
	char	lstate;		
	int	lqhead;		
	int	lqtail;		
	int	ltype;		
	int	lprio;		
	int process_bitmap[NPROC]; 
};
extern	struct	lentry	locks[];
extern	int	nextlock;
extern unsigned long ctr1000;

#endif
