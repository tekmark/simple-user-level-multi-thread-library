/******************************************************************************/
/* CS519 - Operating Systems Design                                           */
/* Author: Hans Christian Woithe                                              */
/******************************************************************************/

#ifndef MY_PTHREAD_H
#define MY_PTHREAD_H

#if MYPTHREAD > 0

#define STACK_SIZE 8192
#define MAX_THREADS 8192

#include <ucontext.h>
//#include <pthread.h>
typedef unsigned int my_pthread_t; /* change this if necessary */
//thread states
enum my_thread_state {
  UNUSED,
  EMBRYO,
  RUNNABLE,
  RUNNING,
  ZOMBIE,
  BLOCKED
};


//thread control block
typedef struct {
  //thread id
  my_pthread_t tid;
  //thread states
  enum my_thread_state state;
  //thread context
  ucontext_t context;
  //return value addr
  long ret;
  //index
  int index;
} my_pthread;

struct my_pthread_queue_element {
  int index;
  struct my_pthread_queue_element* next;
}; 
typedef struct my_pthread_queue_element my_pthread_queue_element_t; 

/*queue for thread*/
typedef struct {
  my_pthread_queue_element_t *q_first; 
  my_pthread_queue_element_t *q_last;
  int q_size; 
} my_pthread_queue;


typedef struct {
  /* Insert code here */
  int lock;                              // 0: free/unlocked, 1: locked, 
  my_pthread_t owner;                    //owener of the mutex;

} my_pthread_mutex_t;

typedef struct {
  /* Insert code here */
  my_pthread_queue   *waiting_queue;     //a queue for blocked threads
  //my_pthread_mutex_t *mutex;             //mutex for queue this queue operation
  int waiting;                           //# of waiting threads
} my_pthread_cond_t;

typedef void my_pthread_attr_t;
typedef void my_pthread_mutexattr_t;
typedef void my_pthread_condattr_t;
/*queue operation*/
void my_pthread_queue_init(my_pthread_queue *);
void my_pthread_queue_enq(my_pthread_queue*, my_pthread*);
my_pthread * my_pthread_queue_deq(my_pthread_queue *);
void my_pthread_queue_destroy(my_pthread_queue *);


#else /* MY_THREAD */

#include <pthread.h>

typedef pthread_t my_pthread_t;
typedef pthread_mutex_t my_pthread_mutex_t;
typedef pthread_cond_t my_pthread_cond_t;
typedef pthread_attr_t my_pthread_attr_t;
typedef pthread_mutexattr_t my_pthread_mutexattr_t;
typedef pthread_condattr_t my_pthread_condattr_t;

#endif

/* thread creation */
int my_pthread_create(my_pthread_t *thread, const my_pthread_attr_t *attr,
		      void *(*start_routine) (void *), void *arg);
void my_pthread_exit(void *retval);
int my_pthread_yield(void);
int my_sched_yield(void); 
int my_pthread_join(my_pthread_t thread, void **retval);

/* mutex */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex,
			  const my_pthread_mutexattr_t *mutexattr);
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex);
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex);
int my_pthread_mutex_trylock(my_pthread_mutex_t *mutex); 
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex);

/* conditional variables */
int my_pthread_cond_init(my_pthread_cond_t *cond,
			 my_pthread_condattr_t *cond_attr);
int my_pthread_cond_signal(my_pthread_cond_t *cond);
int my_pthread_cond_broadcast(my_pthread_cond_t *cond);
int my_pthread_cond_wait(my_pthread_cond_t *cond, my_pthread_mutex_t *mutex);
int my_pthread_cond_destroy(my_pthread_cond_t *cond);
#endif
