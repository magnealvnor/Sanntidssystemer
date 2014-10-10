#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <native/task.h>
#include <native/timer.h>
#include <native/sem.h>
#include <native/mutex.h>
#include <rtdk.h>

/*************************************** USER PROG *************************************/
#include <unistd.h> //usleep
#include <stdlib.h> //malloc, free
#include <pthread.h>
#include <sched.h>

#define CPU_ID          0

#define N_RT_THREADS    2

#define RT_TASK0_PRI    2
#define RT_TASK1_PRI    4
#define RT_TASK2_PRI    6
#define RT_MAIN_PRI     10

#define SLEEP_DELAY    100000000
#define SUCCESS         0

#define METHOD_MUTEX        0
#define METHOD_SEMAPHORE    1

#define USE_METHOD METHOD_SEMAPHORE

static void task0(void *argpointer);
static void task1(void *argpointer);
static void task2(void *argpointer);

static void get_resource(int method, int id);
static void release_resource(int method, int id);
static void sync_threads(int id);
void busy_wait_ms(unsigned long delay);

/* Look-up table for the task funstions */
static void (*const RT_FUNC[N_RT_THREADS]) (void) = {
    task0, task1, task2
};
/* Task priorities */
int RT_THREADS_PRI[]= {RT_TASK0_PRI, RT_TASK1_PRI, RT_TASK2_PRI};

RT_SEM sem, sync_sem;
RT_MUTEX mut;

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

    /* Create semaphore for synchronized start */
    if(rt_sem_create(&sync_sem, "sync_start", 0, S_PRIO) != SUCCESS){
            rt_printf("sync semaphore failed to be created \n");
            exit(1);
        }

    /* Create Barrier */
    #ifdef semaphore
        if(rt_sem_create(&sem, "Barrier_sem", 0, S_PRIO) != SUCCESS){
            rt_printf("Barrier_sem failed to be created \n");
            exit(1);
        }
    #endif
    #ifdef mutex
        if(rt_mutex_create(&mut, "Barrier_mut") != SUCCESS){
            rt_printf("Barrier_mut failed to be created \n");
            exit(1);
        }
    #endif
    /* Initialize Real-Time threads */
    int* id;
	int i;
    RT_TASK rt_threads[N_RT_THREADS];

    for(i=0; i<N_RT_THREADS; i++){
        /* Give the new thread the channel to respond to by argument */
        int *method = malloc(sizeof(int));
        *method = USE_METHOD;
        
        /* Create new thread */
        if(rt_task_create(&rt_threads[i], NULL, 0, RT_THREADS_PRI[i], T_CPU(CPU_ID)|T_JOINABLE) != SUCCESS){
            rt_printf("RT Thread %i failed to be created!\n", i);
            exit(1);
        }

        /* Execute task */
        if(rt_task_start(&rt_threads[i], RT_FUNC[i], id) != SUCCESS){
            rt_printf("RT Thread %i failed to start!\n", (void*) i);
            exit(1);
        }

        /* print success */
        rt_printf("RT Thread  %i initiated successfully \n", i);
    }
    
    /* Wait untill all threads have been blocked, restart them */
    rt_task_sleep(SLEEP_DELAY);
    if(rt_sem_broadcast(&sync_sem)!= SUCCESS){
        rt_printf("main thread failed to broadcast on BARRIER semaphore \n");
        exit(1);
    }
    rt_task_sleep(SLEEP_DELAY);

    /* Wait for all threads to terminate */
    for(i=0; i<N_RT_THREADS; i++){
        rt_task_join(&rt_threads[i]);
    }

    /* Clean up */
    rt_sem_delete(&sem);
    rt_sem_delete(&sync_sem);
    rt_mutex_delete(&mut);

    return 0;
}

static void task0(void *argpointer){
    /* get task method */
    int method = *((int *) argpointer);
    free(argpointer);
    int id = 0;

    sync_threads(id);

    /* get resource */
    get_resource(method, id);

    /* do work */
    rt_printf("Task %i start\n", id);
    busy_wait_ms(3);
    
    /* release resource */
    release_resource(method, id);
}

static void task1(void *argpointer){
    /* get task method */
    int method = *((int *) argpointer);
    free(argpointer);
    int id = 1;

    sync_threads(id);

    rt_task_sleep(1000000);

    rt_printf("Task %i start\n", id);
    busy_wait_ms(5);
    rt_printf("Task %i stop \n", id);
}

static void task2(void *argpointer){
    /* get method */
    int method = *((int *) argpointer);
    free(argpointer);
    int id = 2;

    sync_threads(id);

    rt_task_sleep(2000000);

    /* Get resource */
    get_resource(method, id);
    
    /* Simulate work */
    rt_printf("Task %i start \n", id);
    busy_wait_ms(2);

    /* Release resource */
    release_resource(method, id);
}

static void get_resource(int method, int id){
      if (method == METHOD_SEMAPHORE){
        if(rt_sem_p(&sem, TM_INFINITE) == SUCCESS){
            rt_printf("Task %i got semaphore\n", id);
        }else{
            rt_printf("Task %i failed to get semaphore \n", id); 
        }
    }else if (method == METHOD_MUTEX){
        if(rt_mutex_acquire(&mut, 0) == SUCCESS){
            rt_printf("Task %i got mutex \n", id);
        }else{
            rt_printf("Task %i failed to get mutex \n", id); 
        }
    }
}

static void release_resource(int method, int id){
    if(method == METHOD_SEMAPHORE){
        if(rt_sem_v(&sem) == SUCCESS){
            rt_printf("Task %i unlocked a resource\n", id);
        }else{
            rt_printf("Task %i failed to unlock a resource \n", id); 
        }
    }else if(method == METHOD_MUTEX){
        if(rt_mutex_release(&mut) == SUCCESS){
            rt_printf("Task %i unlocked a resource \n", id); 
        }else{
            rt_printf("Task %i failed to unlock a resource \n", id); 
        }
    }
}

static void sync_threads(int id){
    if(rt_sem_p(&sync_sem, TM_INFINITE) == SUCCESS){
            rt_printf("Task %i got semaphore\n", id);
        }else{
            rt_printf("Task %i failed to get semaphore \n", id); 
        }
}

void busy_wait_ms(unsigned long delay){
    unsigned long count = 0;
    while (count <= delay*10){
        rt_timer_spin(1000*100);
        count++;
    }
}