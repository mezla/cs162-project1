# Initial Design 

## Alarm Lock

### Re-implement void timer_sleep (int64_t ticks)

**Correctness constraints**

    1. Suspends execution of the calling thread until time has advanced by at least x timer ticks. 
    2. Thread need not wake up after exactly x ticks.  Just put it on the ready queue after they are due
    3. Do not fork any additional threads 
    4. Any number of threads may call it and be suspended at any one time.
    5. No busy-waiting

**Data structure**

  * sleeping list: List of threads in sleeping state, the threads will be  waked up until specified duration passed  
  * thread_duration: contains duration and thread pair 
  
**Interface**

  * void timer_sleep (int64_t ticks):

    ```
     current time plus wait time, get the due time.    
     put current thread and due time into waiting queue
     current thread sleeps 
    ``` 
     
  * static void timer_interrupt (struct intr_frame *args UNUSED):

    ```
      iterate the list to check if threads are due
      unblock due thread and remove it from waiting queue    ```
   ```

  * void timer_init (void)
   
    ```
     add a line to initialize waiting queue(list)   
    ```

### Test strategy

  * tests alarm to ensure it waits at least minimum amount of time
  * tests whether alarm actually wakes up at correct time
  * Multi-thread testing

### Test case

  * VAR1: fork one thread, sleep minimum amount of time(1 tick), verify if thread wakes up at correct time
  * vAR2: fork one thread, sleep ZERO tick, verify if thread wakes up at correct time
  * vAR3: fork one thread, sleep negative number of tick, verify if thread wakes up at correct time
  * VAR4: fork one thread, sleep positive number of time, verify if thread wakes up at correct time
  * VAR5: fork several threads, invoke alarm with different time amounts. Verify if all the threads waits at least minimum amount of time and wakes up at correct time.


## Priority Scheduler


### Implementation


**Correctness constraints**

   1. Normally, modify next_thread_to_run () to select the thread with maximum priority in the ready queue.
   2. Consider all the situations that a thread will trigger thread schedule()
       - thread_block, block current thread and schedule next running thread.
       - thread_exit, current therad exits, turns to dying state.
       - thread_yield, current thread gives up CPU but still stays in ready list.
   3. Priority donation
       - handle nested donation
       - implement priority donation for **locks**   

**Data Structure**
   
   * waiting list in thread struct: other threads waiting for this thread.
   * waitedBy list in thread.cL:  other threads which this thread is waiting for ?? Is this list needed?? Should NOT
   * is_dirty flag in thread struct: true, this thread's priority has been updated; false, priority is out of date, need to call update func.
   * effective_priority in thread struct: effective priority after doing donation.

**Interface**

   * **void init_thread (struct thread *t, const char *name, int priority)** 

   ```
      Add below code:

         - set value of effective priority to original priority 
         - set is_dirty flag to flase
         - initialize waiting list 
   ```

   * **struct thread * next_thread_to_run (void)** 

   ```
     Add below code: 

         - loop threads in ready_list and select the thread with maximum effective priority as next running thread 
	 - remove selected next thread from ready_list
   ```

   * **void thread_set_priority (int new_priority)** 

   ```
         - Sets the current thread's priority to new_priority.  
         - If the current thread no longer has the highest priority, yields.
   ```
 
  * **int thread_get_priority (void)** : 

  ```
         - If this thread is dirty, call this function recursively to update donated priority and return it.
         - Else return current priority.
  ```
 
  * lock_acquire (struct lock *lock)

  ```
         - append semaphore->waiters into current thread's waitingFor list
         - mark this thread dirty (so effective priority will be update in next scheduling)
  ```
  
  * lock_release(struct lock *lock)
 
  ``` 
         - remove threads in semaphore->waiters from current thread's waiting list
         - remove current thread from waiting thread's waited's list
         - mark current thread dirty if effective priority is unequal with original priority
  ```
 
###  Test strategy

Phase I:

   * Test ThreadGrader5.a: Tests priority scheduler without donation
   * Test ThreadGrader5.c: Tests priority scheduler without donation, altering priorities of threads after they've started running

Phase II:

   * Test ThreadGrader6a.a: Tests priority donation
   * Test ThreadGrader6a.b: Tests priority donation with more locks and more complicated resource allocation

### Test case

   * Test ThreadGrader5.a: Tests priority scheduler without donation
      - VAR1: MyTester.PriopritySchedulerVAR1. Create several(>2) threads, verify these threads can be run successfully.
      - VAR2: MyTester.PriopritySchedulerVAR2. Create lots of threads with more locks and more complicated resource allocation

   * Test ThreadGrader5.c: Tests priority scheduler without donation, altering priorities of threads after they've started running
      - VAR3: MyTester.PriopritySchedulerVAR3. Create several(>2) threads, decrease or increase the priorities of these threads, verify these threads can be run successfully.
      - VAR4: MyTester.PriopritySchedulerVAR4. Create a scenario to hit the priority inverse problem. Verify the highest thread is blocked by lower priority thread.

   * Test ThreadGrader6a.a: Tests priority donation
      - VAR: Tested by VAR4

   * Test ThreadGrader6a.b: Tests priority donation with more locks and more complicated resource allocation
      - VAR: Tested by Other cases.


## 4BSD Scheduler


### Implementation


**Correctness constraints**

   * The 4.4BSD scheduler does not include priority donation.
   * data updates should occur before any ordinary kernel thread has a chance to run.
   * positive nice, to the maximum of 20, decreases the priority of a thread and causes it to give up some CPU time it would otherwise receive.
   * a negative nice, to the minimum of -20, tends to take away CPU time from other threads. 
   * The initial thread starts with a nice value of zero.
   * Thread priority is calculated initially at thread initialization.  It is also recalculated once every fourth clock tick.
   * Calculate priority: **priority=PRI_MAX-(recent_cpu/4)-(nice*2)**

**Data structure**

**Interface**
   * *set_priority*: calculate priority

   * *int thread_get_nice (void)*: Returns the current thread's nice value.

   * *void thread_set_nice (int new_nice)*: Sets the current thread's nice value to new nice and recalculates the thread's priority based on the new value.

   * *int thread_get_recent_cpu(void)*: Returns 100 times the current thread's recent_cpu value, rounded to the nearest integer.
  
    ```
     1> priority = PRI_MAX - (recent_cpu/4) - (nice * 2)  
     2> recent_cpu = (2 * load_avg)/(2 * load_avg + 1) * recent_cpu + nice
     3> load_avg = (59/60) * load_avg + (1/60) * ready_threads
         (load_avg was initialized to 0 at boot and recalculated once per second) 
    ```  
