/******************************************************************************/
/* CS519 - Operating Systems Design                                           */
/* Author: Hans Christian Woithe                                              */
/******************************************************************************/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "mypthread.h"
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <ucontext.h>
#include <unistd.h>

#if MYPTHREAD > 0

extern void my_pthread_begin();
extern void my_pthread_end();

/* Insert code here */

#define THREAD_INTERVAL 100000

static my_pthread *threads[MAX_THREADS];
static my_pthread *current_thread;
static int is_main_thread = 1;
static int thread_index = 0;
static int available_tid = 1;

void my_scheduler();

#if PREEMPTIVE > 0                                                 //if use timer signal;
void thread_handler(int signal, siginfo_t *info, void *data) {
  ucontext_t *uc = (ucontext_t*)data;                               //get context
  if( uc->uc_mcontext.gregs[REG_RIP] > (long)my_pthread_begin &&
         uc->uc_mcontext.gregs[REG_RIP] < (long)my_pthread_end) {   //if signal happens in app
    
    my_pthread_yield();                                                     //schedule
  } else {                                                                   //else do nothing
  }
  //do nothing if the timer expired
}
#endif

/*my thread creation*/
int my_pthread_create(my_pthread_t *thread, const my_pthread_attr_t *attr,
                     void *(*start_routine) (void *), void *arg) 
{
  printf("my_pthread_create\n");
  if( thread_index >= MAX_THREADS ) {
    perror("Cannot create thread because # of threads exceeds MAX_THREAD\n");
    return -1;
  }
  if( is_main_thread ){  //main thread
    printf("create main thread\n");

#if PREEMPTIVE > 0             //if use preemptive, resigter signal handler and set tiemr
    struct sigaction act;
    act.sa_sigaction=thread_handler;
    sigemptyset(&act.sa_mask); 
    act.sa_flags = SA_SIGINFO;   //use 2nd&3rd parameters in signal handler
    sigaction(SIGALRM, &act, NULL);
    //set interval, and restart timer after previous timer expires
    struct itimerval interval;
    interval.it_interval.tv_sec = 0;
    interval.it_interval.tv_usec = THREAD_INTERVAL;
    interval.it_value.tv_sec = 0;   
    interval.it_value.tv_usec = THREAD_INTERVAL;
    setitimer(ITIMER_REAL, &interval,0);  
#endif
    
    //thread_index is 0;
    threads[thread_index] = malloc(sizeof( my_pthread));
    //tid of main thread is 1
    threads[thread_index]->tid = available_tid;
    available_tid++;
    threads[thread_index]->state = RUNNING;
    getcontext(&threads[thread_index]->context);
    threads[thread_index]->index = thread_index;    //should be 0
    //threads[thread_index]->context.uc_stack.ss_sp = malloc( sizeof(char) * STACK_SIZE );
    //threads[thread_index]->context.uc_stack.ss_size = sizeof(char) * STACK_SIZE; 

    current_thread = threads[thread_index];
    is_main_thread = 0;                         //only one main thread;
    thread_index++;
  }
  if( is_main_thread == 0) {
    printf("create child thread \n"); 
  }
  threads[thread_index] = malloc(sizeof(my_pthread));
  threads[thread_index]->tid = available_tid;
  available_tid++;
  *thread = threads[thread_index]->tid;
  getcontext(&threads[thread_index]->context);
  //create a stack for the new stack
  threads[thread_index]->context.uc_stack.ss_sp = malloc(sizeof(char) * STACK_SIZE); 
  threads[thread_index]->context.uc_stack.ss_size = sizeof(char) * STACK_SIZE;
  //contect to be resumed
  threads[thread_index]->context.uc_link = &current_thread->context;

  makecontext(&threads[thread_index]->context, (void *) start_routine, 1, arg);
  //make the thread runnable
  threads[thread_index]->state = RUNNABLE;
  threads[thread_index]->index = thread_index;
  thread_index++;
  return 0;
}

void my_pthread_exit(void *retval)
{ 
  long value; 
  value = (long) retval;  
  current_thread->state = ZOMBIE;
  current_thread->ret = value;
  my_scheduler(); 
}

int my_pthread_yield(void)
{
  current_thread->state = RUNNABLE;
  my_scheduler();
  
  return 0; 
}
int my_sched_yield(void){
  return my_pthread_yield();
}

void my_scheduler() {
  my_pthread *temp; 
  int i; 
  i = current_thread->index;
  //find a runnable thread
  while(1) {
    i++;
    if( i == thread_index ){      //back to zero
      i = 0; 
    }
    if( threads[i]->state == RUNNABLE || threads[i] != current_thread) {  //find a runnable thread
      break;
    }
  }
  
  threads[i]->state = RUNNING;                      //change the state scheduled thread to running;
  temp = current_thread;
  current_thread = threads[i];
  //save old thread context, and activate new thread context
  swapcontext(&temp->context, &current_thread->context); 
}


int my_pthread_join(my_pthread_t thread, void **retval)
{ 
  int i;
  int check_flag = 0;   // 0: no such thread

  //find target thread
  while(1) {
    check_flag = 0; 
    for( i = 0; i != thread_index; i++) {
      if( threads[i]->tid != thread) {            //if tid doesn't match, next thread; 
        continue; 
      } else if ( threads[i]->state == ZOMBIE ) { //if tid matches, but pthread_exit() has executed.
        check_flag = 1;
        *(long **)retval = (long *)threads[i]->ret;
        threads[i]->state = UNUSED;               //set thread unused
        threads[i]->tid = 0;     
        //free the stack created here              
        return 0;                                 //return successful;
      } else {                                    //if tid matches, and target thread is runnalbe
        check_flag = 1;
      } 
    }  
    if ( check_flag == 0 ) {
      return -1;                                //error, no such flag; 
    }
    my_pthread_yield();                         //current thread yield, until find the zombie target thread  
  }
}

/*my mutex*/
int my_pthread_mutex_init(my_pthread_mutex_t *mutex,
                          const my_pthread_mutexattr_t *mutexattr)
{ 
  if(!mutex) {
    printf("failed to initialize a mutex, because of NULL pointer\n"); 
    return -1;
  }
  mutex->lock = 0;                         //set lock free
  mutex->owner = (my_pthread_t)-1;         //set no owner;
  return 0; 
}

int my_pthread_mutex_lock(my_pthread_mutex_t *mutex)
{
  while(1) {                              //spin
    if(mutex->lock == 0) {                //if free
      mutex->lock = 1;                    //lock it
      mutex->owner = current_thread->tid; //and set current thread as its owner
      return 0; 
    } 
  } 
}

int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex)
{
  while( mutex->lock != 0 ) {                  //while mutex locked
    if(mutex->owner == current_thread->tid){   //if tid matches
      mutex->lock = 0;                         //unlock it
      mutex->owner = (my_pthread_t)-1;         //no owner; 
      return 0; 
    }   
  }
  return 0;                                    //if lock if free, return
}

int my_pthread_mutex_trylock(my_pthread_mutex_t *mutex) 
{
  if( mutex->lock == 0) {               
    mutex->lock = 1;                    //lock if free;
    mutex->owner = current_thread->tid; //set owner;
    return 0;                           //return 
  } else {                              //return -a if locked
    return -1;                          
  }
}


int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex)
{ 
  if( mutex != NULL) 
    free(mutex);
  return 0; 
}
/* my conditional variables */
int my_pthread_cond_init(my_pthread_cond_t *cond,
                         my_pthread_condattr_t *cond_attr)
{ 
  cond->waiting = 0; 
  cond->waiting_queue = malloc(sizeof(my_pthread_queue));   //create a queue, which needs free;  
  my_pthread_queue_init( cond->waiting_queue );
  return 0; 
}

int my_pthread_cond_signal(my_pthread_cond_t *cond)
{ 
  if (cond->waiting > MAX_THREADS ) {           //if # of waiting threads exceeds MAX_THREADS
    return -1;                                  //return error;
  } 
  else if (cond->waiting == 0) {
    return 0;                                   //no thread waiting for this cond, return
  } else {                                      //if some threads are waiting for this cond
    cond->waiting--;
    my_pthread *temp; 
    temp = my_pthread_queue_deq(cond->waiting_queue); //dequeue this thread from waiting list
    if(temp == NULL) {
      return -1;                                //this should not happen
                                                //because cond->waiting is checked before; 
    }
    threads[temp->index]->state = RUNNABLE;     //make that thread runnable,
                                                //so it may get scheduled.
    //my_pthread_yield(); 
    return 0;                                   
  }
  
}

int my_pthread_cond_broadcast(my_pthread_cond_t *cond)
{ 
  if( cond->waiting > MAX_THREADS ) {
    return -1; 
  }
  else if(cond->waiting == 0) {
    return 0;
  } else {
    my_pthread *temp; 
    while( cond->waiting ){                       //release all blocked threads
      temp = my_pthread_queue_deq( cond->waiting_queue);
      if ( temp == NULL ) {
        return -1; 
      }
      threads[temp->index]->state = RUNNABLE;
      cond->waiting--; 
    }
    return 0; 
  }
}

int my_pthread_cond_wait(my_pthread_cond_t *cond, my_pthread_mutex_t *mutex)
{ 
  int rc = 0, index = 0;
  cond->waiting++;                                //add waiting #
  
  index = current_thread->index;                  //find calling thread
  threads[index]->state = BLOCKED;                //change its state to block
  
  //enqueue the blocked thread to condition waiting queue
  my_pthread_queue_enq(cond->waiting_queue, threads[index]);
  rc = my_pthread_mutex_unlock(mutex);            //unlock locked mutex, so other thread can use
                                                  //the data this mutex locked.
  if( rc != 0) {
     return -1;                                   //return error, if connot unlock the mutex
  }

  my_pthread_yield();                             //yield to other runnable thread;
                                                  //because of this thread is block;
                                                  //it cannot get processor until, pthread_cond_signal 
                                                  //change its state to RUNNABLE;
  rc = my_pthread_mutex_lock(mutex);              //after get processor, lock the mutex;
  if( rc != 0) {                                 
    return -1;                                    //return -1, if cannot lock the mutex; 
  }
  return 0;                                       //success, return 0;                   
}

int my_pthread_cond_destroy(my_pthread_cond_t *cond)
{ 
  if(cond != NULL) {
    if(cond->waiting_queue != NULL) {
      free(cond->waiting_queue);
    }
    free(cond);
  }
  return 0; 
}

/* queue operations */
void my_pthread_queue_init(my_pthread_queue* queue)
{ 
  queue->q_first = NULL;
  queue->q_last = NULL;
  queue->q_size = 0;
}

void my_pthread_queue_enq(my_pthread_queue* queue, my_pthread *thread){
  my_pthread_queue_element_t *element = malloc(sizeof(my_pthread_queue_element_t)); //note: malloc, need delete
  element->index = thread->index;
  element->next = NULL;
  if( queue->q_last == NULL ) {           //no element in the queue
     queue->q_last = element;             //both first and last point to this element
     queue->q_first = element;
     queue->q_size++; 
  } else {                                //if not a empty queue
     queue->q_last->next = element;
     queue->q_last = element; 
     queue->q_size++; 
  }
}

my_pthread * my_pthread_queue_deq(my_pthread_queue * queue) {
  int ret;
  if( queue->q_size == 0) {
    return NULL;                          // No elements in the queue
  } else if ( queue->q_size == 1 ) {      // if only one element in the queue, which means
                                          // both first and last point to the same element
    ret = queue->q_first->index;
    free(queue->q_first);                   //free memory
    queue->q_first = NULL;
    queue->q_last =NULL;
    queue->q_size = 0;
  } else {                                  //if there are more than one element in the queue
    my_pthread_queue_element_t *temp;
    temp = queue->q_first;
    ret = queue->q_first->index;            //
    queue->q_first = queue->q_first->next;
    queue->q_size--; 
    free(temp);                            //free dequeued element 
  }
  return threads[ret];  
}

void my_pthread_queue_destroy(my_pthread_queue * queue) {
  if(queue != NULL) 
    free(queue); 
}

#else

/* thread creation */
int my_pthread_create(my_pthread_t *thread, const my_pthread_attr_t *attr,
		      void *(*start_routine) (void *), void *arg)
{
	return pthread_create(thread, attr, start_routine, arg);
}

void my_pthread_exit(void *retval)
{
	pthread_exit(retval);
}

int my_pthread_yield(void)
{
	return pthread_yield();
}

int my_pthread_join(my_pthread_t thread, void **retval)
{
	return pthread_join(thread, retval);
}

/* mutex */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex,
			  const my_pthread_mutexattr_t *mutexattr)
{
	return pthread_mutex_init(mutex, mutexattr);
}

int my_pthread_mutex_lock(my_pthread_mutex_t *mutex)
{
	return pthread_mutex_lock(mutex);
}

int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex)
{
	return pthread_mutex_unlock(mutex);
}

int my_pthread_mutex_trylock(my_pthread_mutex_t *mutex)
{
        return pthread_mutex_trylock(mutex); 
}

int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex)
{
	return pthread_mutex_destroy(mutex);
}

/* conditional variables */
int my_pthread_cond_init(my_pthread_cond_t *cond,
			 my_pthread_condattr_t *cond_attr)
{
	return pthread_cond_init(cond, cond_attr);
}

int my_pthread_cond_signal(my_pthread_cond_t *cond)
{
	return pthread_cond_signal(cond);
}

int my_pthread_cond_broadcast(my_pthread_cond_t *cond)
{
	return pthread_cond_broadcast(cond);
}

int my_pthread_cond_wait(my_pthread_cond_t *cond, my_pthread_mutex_t *mutex)
{
	return pthread_cond_wait(cond, mutex);
}

int my_pthread_cond_destroy(my_pthread_cond_t *cond)
{
	return pthread_cond_destroy(cond);
}

#endif
