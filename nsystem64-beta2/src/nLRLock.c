#include "nSysimp.h"
#include "fifoqueues.h"

typedef struct {
	int left, right;
	int sl;
	FifoQueue queue;
} *nLRLock;

/* Evite que se defina nLRLock como void * en nSystem.h */
#define NOVOID_NLRLOCK

#include "nSystem.h"

nLRLock nMakeLeftRightLock() {
	nLRLock lock = (nLRLock)nMalloc(sizeof(*lock));
	lock->queue = MakeFifoQueue();
	lock->left = 0;
	lock->right = 0;
	return lock;
}

int nHalfLock(nLRLock l, int timeout) {
	int res;
	if (timeout < 0) {
		START_CRITICAL();
  		if (l->left && l->right) {
        current_task->status = WAIT_TASK;
  			PutObj(l->queue, current_task);
  			END_CRITICAL();
  			ResumeNextReadyTask();
  			START_CRITICAL();
  		}
  		if (l->left) {
  			l->right = 1;
  			res = RIGHT;
  		} else {
  			l->left = 1;
  			res = LEFT;
  		}
  		END_CRITICAL();
  	} else {
  		START_CRITICAL();
  		if (l->left && l->right) {
  			PutObj(l->queue, current_task);
  			END_CRITICAL();
  			ResumeNextReadyTask();
  			START_CRITICAL();
  		}
  		if (l->left) {
  			l->right = 1;
  			res = RIGHT;
  		} else {
  			l->left = 1;
  			res = LEFT;
  		}
  		END_CRITICAL();
  	}
  	return res;
}

void nHalfUnlock(nLRLock l, int side) {
	nTask next;
	START_CRITICAL();
	if (side == LEFT) {
		l->left = 0;
	} else {
		l->right = 0;
	}
	next = GetObj(l->queue);
  if (next != NULL) {
    next->status = READY;
  	PutTask(ready_queue, next);
  }
  END_CRITICAL();
}

int nFullLock(nLRLock l, int timeout) {
	int res;
	if (timeout < 0) {
		  START_CRITICAL();
  		
      if (l->left || l->right) {
        current_task->status = WAIT_TASK;
        PutObj(l->queue, current_task);
        END_CRITICAL();
        ResumeNextReadyTask();
        START_CRITICAL();
  		}

  		l->right = 1;
  		l->left = 1;
  		res = TRUE;
  		END_CRITICAL();
  	} else {
  		START_CRITICAL();
  		if (l->left || l->right) {
        current_task->status = WAIT_TASK;
        PutObj(l->queue, current_task);
        END_CRITICAL();
        ResumeNextReadyTask();
        START_CRITICAL();
  		}
		l->right = 1;
		l->left = 1;
		res = TRUE;
  		END_CRITICAL();
  	}
  	return res;
}

void nFullUnlock(nLRLock l) {
	nTask next;
	START_CRITICAL();
	l->left = 0;
	l->right = 0;
  next = GetObj(l->queue);
  if (next != NULL) {
    next->status = READY;
    PutTask(ready_queue, next);
  }
  END_CRITICAL();
}

void nDestroyLeftRightLock(nLRLock l) {
	DestroyFifoQueue(l->queue);
	nFree(l);
}
