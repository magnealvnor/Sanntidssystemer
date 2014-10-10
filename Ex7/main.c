#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <native/task.h>
#include <native/timer.h>
#include <native/sem.h>
#include <rtdk.h>

/*************************************** USER PROG *************************************/
#include <unistd.h> //usleep
#include <stdlib.h> //malloc, free
#include <pthread.h>
#include <sched.h>

#define CPU_ID          0

#define N_RT_THREADS    3

#define RT_TASK0_PRI    2
#define RT_TASK1_PRI    4
#define RT_TASK3_PRI    6

#define RT_MAIN_PRI     10

#define SLEEP_DELAY    100000000
#define SUCCESS         0

static void *RT_function(void *argpointer);
const int RT_THREADS_PRI[]= {RT_TASK0_PRI, RT_TASK1_PRI, RT_TASK2_PRI};
RT_SEM BARRIER;

int main(void){

    /* Perform auto-init of rt_print buffers if the task doesn't do so */
    rt_print_auto_init(1);

    /* Lock memory : avoid memory swapping for this program */
    mlockall(MCL_CURRENT|MCL_FUTURE);
    
    /* Make main a Real-Time thread */
    if(rt_task_shadow(NULL, NULL, RT_MAIN_PRI, T_CPU(CPU_ID) ) != SUCCESS){
        printf("RT Thread main failed to be created!\n");
        exit(1);
    }else{
        /* print success */
        rt_printf("RT Thread main initiated successfully \n");
    }

    /* Create Barrier */
    if(rt_sem_create(&BARRIER, "Barrier", 0, S_PRIO) != SUCCESS){
        rt_printf("BARRIER semaphore failed to be created \n");
        exit(1);
    }

    /* Initialize Real-Time threads */
    int* chan;
	int i;
    RT_TASK rt_threads[N_RT_THREADS];

    for(i=0; i<N_RT_THREADS; i++){
        /* Give the new thread the channel to respond to by argument */
        chan = malloc(sizeof(int));
        *chan = i;
        
        /* Create new thread */
        if(rt_task_create(&rt_threads[i], NULL, 0, RT_THREADS_PRI[i], T_CPU(CPU_ID)|T_JOINABLE) != SUCCESS){
            rt_printf("RT Thread %i failed to be created!\n", i);
            exit(1);
        }

        /* Execute task */
        if(rt_task_start(&rt_threads[i], RT_function, chan) != SUCCESS){
            rt_printf("RT Thread %i failed to start!\n", (void*) i);
            exit(1);
        }

        /* print success */
        rt_printf("RT Thread  %i initiated successfully \n", i);
    }
    
    /* Wait untill all threads have been blocked, restart them */
    rt_task_sleep(SLEEP_DELAY);
    if(rt_sem_broadcast(&BARRIER)!= SUCCESS){
        rt_printf("main thread failed to broadcast on BARRIER semaphore \n");
        exit(1);
    }
    rt_task_sleep(SLEEP_DELAY);

    /* Collect all threads */
    for(i=0; i<N_RT_THREADS; i++){
        rt_task_join(&rt_threads[i]);
    }

    /* Clean up */
    rt_sem_delete(&BARRIER);

    return 0;
}

/* Polls the signal line given by argument and reacts on the corresponding line */
static void *RT_function(void *argpointer){

    int chan = *((int *) argpointer);
    free(argpointer);

    if(rt_sem_p(&BARRIER, TM_INFINITE) == SUCCESS){
        rt_printf("Task %i freed\n", chan);
    }else{
        rt_printf("Task %i failed to get semaphore \n", chan); 
    }
}
