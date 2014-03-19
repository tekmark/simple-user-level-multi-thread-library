#include "mypthread.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM_OF_ELEMENTS 10
#define NUM_OF_THREADS NUM_OF_ELEMENTS - 1

void my_pthread_begin() {} /* no code before this */

/***********test case for my_pthread conditional variable*********begin*/
/*
int                    workToDo = 0;
my_pthread_cond_t      *cond;   //= PTHREAD_COND_INITIALIZER;
my_pthread_mutex_t     *mutex;  //= PTHREAD_MUTEX_INITIALIZER;

#define NTHREADS      2

void *threadfunc(void *parm)
{
  while (1) {
    my_pthread_mutex_lock(mutex);

    while (!workToDo) {
      printf("Thread blocked\n");
      my_pthread_cond_wait(cond, mutex);
    }
    printf("Thread awake, finish work!\n");

    workToDo = 0;

    my_pthread_mutex_unlock(mutex);
    my_pthread_yield(); 
  }
  my_pthread_exit(NULL); 
  return NULL;
}

int main(int argc, char **argv)
{
  int                   rc=0;
  int                   i;
  my_pthread_t          threadid[NTHREADS];
  //initialization
  cond = malloc(sizeof(my_pthread_cond_t)); 
  my_pthread_cond_init(cond, NULL);
  mutex = malloc(sizeof(my_pthread_mutex_t));
  my_pthread_mutex_init(mutex, NULL); 
  
  printf("Create %d threads\n", NTHREADS);
  for(i=0; i<NTHREADS; ++i) {
    rc = my_pthread_create(&threadid[i], NULL, threadfunc, (void*)cond);
    if ( rc != 0) {
      printf("fail to create a child thread\n");
      exit(0); 
    }
  }
  my_pthread_yield(); 
  sleep(5);                  // Sleep is not a very robust way to serialize threads

  for(i=0; i<100; ++i) {
    printf("Wake up a worker, work to do...\n");

    rc = my_pthread_mutex_lock(mutex);

    if (workToDo) {
       printf("Work already present, likely threads are busy\n");
    }
    workToDo = 1;
    my_pthread_cond_signal(cond);
    if ( rc != 0 ) {
      printf("fail to signal cond\n"); 
    }

    my_pthread_mutex_unlock(mutex);
    my_pthread_yield(); 
    sleep(5);  
  }
  
  printf("Main completed\n");
  my_pthread_exit(NULL);
  my_pthread_cond_destroy(cond);
  my_pthread_mutex_destroy(mutex); 
  printf("Memory freed\n"); 
  exit(0);
  return 0;
}
*/
/**********************test case for conditional variable*******end********/

/**********************test case for PartA**********begin******************/
/* Insert code here */

struct thread_data_st {
  int *array;                       //  size n+1;
  int pos;                          //  0 to n-1;
  my_pthread_mutex_t *mtx; 
}; 

void *swap(void *argument) {
  struct thread_data_st *data = (struct thread_data_st*)argument;
  int *a = data->array;
  int index = data->pos;
  my_pthread_mutex_t *mtx = data->mtx;
  int temp;
  
  while(1){
    my_pthread_mutex_lock(&mtx[index]);
    if( my_pthread_mutex_trylock(&mtx[index + 1]) != 0){    //if mutex[index+1] is locked
      my_pthread_mutex_unlock(&mtx[index]);              //unlock mutex[index], schedule next
      my_pthread_yield(); 
      continue; 
    }
    if( a[index] > a[index+1] ){
      //printf("swap******************\n"); 
      temp = a[index];                                  //swap
      a[index] = a[index+1];
      a[index + 1] = temp; 
    }
    my_pthread_mutex_unlock(&mtx[index]); 
    my_pthread_mutex_unlock(&mtx[index+1]); 
    my_pthread_yield(); 
  }
  my_pthread_exit(NULL);
}

struct array_st {
  int *ptr;
  int size;
};  

int main(int argc, char *argv[])
{
  printf("HW3 CS519\n");
  
  int a[NUM_OF_ELEMENTS]; // = {9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
  int rc, i; 
  my_pthread_t threads[NUM_OF_THREADS];
  my_pthread_mutex_t mutex[NUM_OF_ELEMENTS];
  for ( i = 0; i != NUM_OF_ELEMENTS; i++) {
    my_pthread_mutex_init(&mutex[i], NULL); 
  }

  struct thread_data_st thread_args[NUM_OF_THREADS];  

  for(i = 0; i != NUM_OF_ELEMENTS; i++) {
    a[i] = NUM_OF_ELEMENTS-i-1;
  }
  
  //rc = pthread_create(&check_thread, NULL, check_sorted, (void*)&arr); 
  for( i = 0; i != NUM_OF_THREADS; i++) {
    thread_args[i].array = a;
    thread_args[i].pos = i;
    thread_args[i].mtx = mutex;
    rc = my_pthread_create(&threads[i], NULL, swap, (void*)&thread_args[i]);
    if( rc != 0) {
      printf("fatal: cannot create thread\n"); 
    }
  }
  
  int sorted = 0; //unsorted
  
  while(!sorted) {
    for( i = 0; i != NUM_OF_ELEMENTS; i++) {
      my_pthread_mutex_lock(&mutex[i]);   
    }
    sorted = 1; 
    for(i = 1; i != NUM_OF_ELEMENTS; i++) {
      if( a[i -1] > a[i] ) { 
        sorted = 0;
        break;
      }
    }
    for( i = 0; i != NUM_OF_ELEMENTS; i++) {
      my_pthread_mutex_unlock(&mutex[i]); 
    }
    //my_sched_yield();
    my_pthread_yield(); 
    //sleep(1); 
  }
  
  printf("sorted \n");
  for( i = 0; i != NUM_OF_ELEMENTS ; i++) {
    printf("%u ", a[i]); 
  }
  
  for( i = 1; i != NUM_OF_ELEMENTS; i++) {
    if(a[i-1] > a[i]) {
      printf("error!!!!!!!!!!!!!!!!!!!!!!\n"); 
    }
  }
  
  printf("\n");
  exit(0);
  
  //pthread_exit(NULL);
  //return 0;
}

/********************test case for PartA********************end******************/
/* Insert code here */
/*
void *run(void *para) {
  int *v = (int*)para;
  printf("hello v %u\n", *v); 
  //my_pthread_exit(NULL); 
  int *a = malloc(sizeof(int));
  *a = 12;  
  return NULL; 
}

int main( int argc, char *argv[]) {
  my_pthread_t thread;   
  int rc; 
  int v = 1; 
  rc = my_pthread_create(&thread, NULL, (void*)run, (void*)&v); 
  if( rc < 0) {
    printf("cannot create thread\n"); 
  }
  my_pthread_join(thread, NULL);
  printf("main thread quit, ret\n"); 
  return 1; 
}
*/

void my_pthread_end() {} /* no code after this */
