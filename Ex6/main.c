#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <native/task.h>
#include <native/timer.h>
#include <rtdk.h>

#define TEST_MODE false

#if TEST_MODE
/****************************************** TEST PROG******************************************/
RT_TASK demo_task;

void demo(void *arg){
    RT_TASK *curtask;
    RT_TASK_INFO curtaskinfo;

    /* hello world */
    rt_printf("Hello World!\n");

    /* inquire current task */
    curtask=rt_task_self();

    rt_task_inquire(curtask,&curtaskinfo);

    /* print task name */
    rt_printf("Task name : %s \n", curtaskinfo.name);
}

int main(int argc, char* argv[]){
    char  str[10] ;

    /* Perform auto-init of rt_print buffers if the task doesn't do so */
    rt_print_auto_init(1);

    /* Lock memory : avoid memory swapping for this program */
    mlockall(MCL_CURRENT|MCL_FUTURE);

    rt_printf("start task\n");
    sprintf(str,"hello");
 

    /*
    * Arguments: &task,
    *            name,
    *            stack size (0=default),
    *            priority,
    *            mode (FPU, start suspended, ...)
    */
    rt_task_create(&demo_task, str, 0, 50, 0);

 
    /*
    * Arguments: &task,
    *            task function,
    *            function argument
    */
    rt_task_start(&demo_task, &demo, 0);
}

#else
/*************************************** USER PROG *************************************/
#include <unistd.h> //usleep
#include <stdlib.h> //malloc, free
#include <pthread.h>
#include <sched.h>
#include "io.h"

#define N_RT_THREADS    3
#define N_PTHREADS      10
#define RT_THREADS_PRI  50
#define SLEEP_PERIOD    9000
#define SUCCESS         0

static void *RT_function(void *argpointer);
static void *pthread_function(void *argpointer);
int set_cpu(int cpu_number);

int main(void){
    
    /* Perform auto-init of rt_print buffers if the task doesn't do so */
    rt_print_auto_init(1);

    /* Lock memory : avoid memory swapping for this program */
    mlockall(MCL_CURRENT|MCL_FUTURE);

    /* init hardware */
    int ret=io_init();
    if(ret < 1){
        printf("io init failed");
        return 1;
    } 

    /* Initialize Real-Time threads */
    int* chan;
	int i;
    RT_TASK rt_threads[N_RT_THREADS];
    for(i=0; i<N_RT_THREADS; i++){
        /* Give the new thread the channel to respond to by argument */
        chan = malloc(sizeof(int));
        *chan = i+1;
        
        /* Create new thread */
        if(rt_task_create(&rt_threads[i], NULL, 0, RT_THREADS_PRI, 0) != SUCCESS){
            printf("RT Thread %i failed to be created!\n", i);
            exit(1);
        }

        /* Task is to execute periodicly */
        if(rt_task_set_periodic(&rt_threads[i], TM_NOW, SLEEP_PERIOD) != SUCCESS){ 
            printf("RT Thread %i failed to set period!\n", i);
            exit(1);
        }

        /* Execute task */
        if(rt_task_start(&rt_threads[i], RT_function, chan) != SUCCESS){
            printf("RT Thread %i failed to start!\n", (void*) i);
            exit(1);
        }

        /* print success */
        printf("RT Thread  %i initiated successfully \n", i);
    }

    /* Initialize disturbance threads */
    pthread_t pthreads[N_PTHREADS];

    for(i=0; i<N_PTHREADS; i++){

        /* Create new thread */
        if(pthread_create(&pthreads[i], NULL, pthread_function, NULL) != SUCCESS){
            printf("Disturbance thread %i failed to be created \n", i);
            exit(1);
        }

        /* print success */
        printf("Pthread %i initiated successfully \n", i);
    }

    /* Collect threads before exiting */
	int j;
    for(j = 0; j < N_PTHREADS; j++){
        pthread_join(pthreads[j], NULL);
    }   

    return 0;
}

/* Polls the signal line given by argument and reacts on the corresponding line */
static void *RT_function(void *argpointer){
    //set cpu

    int chan = *((int *) argpointer);
    free(argpointer);

    while(1){
        rt_task_wait_period(NULL);

        if(io_read(chan) == 0){
            io_write(chan, 0);
            usleep(5);
            io_write(chan, 1);
        }
    }   
}

static void *pthread_function(void *argpointer){
    set_cpu(1);
    int retardation_lvl;

    while(1){
        retardation_lvl++;
        
        if(retardation_lvl>9000){
            retardation_lvl = 0;
        }
    }
}

int set_cpu(int cpu_number)
{
	/* setting cpu set to the selected cpu */
	cpu_set_t cpu;
	CPU_ZERO(&cpu);
	CPU_SET(cpu_number, &cpu);

	/* set cpu set to current thread and return */
	return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t),&cpu);
}

#endif
