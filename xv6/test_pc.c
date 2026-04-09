#include "types.h"
#include "stat.h"
#include "user.h"
#include "uthread.h"
#include "umutex.h"

#define N 8
static int buf[N], head, tail, count;
static umutex_t mu;

static int total_produced = 0;
static int total_consumed = 0;

static void producer(void *arg){
  int id = (int)arg;
  for(int i = 0; i < 100; i++){
    while(1){
      mutex_lock(&mu);
      if(count < N) break;
      mutex_unlock(&mu);
      thread_yield();
    }
    buf[tail] = id*1000 + i;
    tail = (tail+1)%N;
    count++;
    total_produced++;
    mutex_unlock(&mu);
    thread_yield();
  }
  printf(1, "producer %d: done\n", id);
}

static void consumer(void *arg){
  int id  = (int)arg;
  int got = 0;
  while(got < 100){
    while(1){
      mutex_lock(&mu);
      if(count > 0) break;
      mutex_unlock(&mu);
      thread_yield();
    }

    int x = buf[head];
    head = (head+1)%N;
    count--;
    total_consumed++;
    got++;
    mutex_unlock(&mu);
    if(got % 25 == 0) printf(1, "consumer %d: got %d (last=%d)\n", id, got, x);
    thread_yield();
  }
  printf(1, "consumer %d: done\n", id);
}

int main(void){
  thread_init();
  mutex_init(&mu);

  tid_t p1 = thread_create(producer, (void*)1);
  tid_t p2 = thread_create(producer, (void*)2);
  tid_t c1 = thread_create(consumer, (void*)1);
  tid_t c2 = thread_create(consumer, (void*)2);

  if(p1 < 0 || p2 < 0 || c1 < 0 || c2 < 0){
    printf(2, "thread_create failed\n");
    exit();
  }

  thread_join(p1); 
  thread_join(p2);
  thread_join(c1);
  thread_join(c2);

  printf(1, "produced : %d  (want 200)\n", total_produced);
  printf(1, "consumed : %d  (want 200)\n", total_consumed);
  printf(1, "count    : %d  (want   0)\n", count);

  if(total_produced == 200 && total_consumed == 200 && count == 0)
    printf(1, "PASS\n");
  else
    printf(1, "FAIL\n");

  exit();
}
