#include <native/sem.h>
#include <native/task.h>
#include <stdio.h>
#include <sys/mman.h>
#include <rtdk.h>
#include <native/timer.h>
#include <native/mutex.h>

//#define semaphore
#define mutex

static RT_TASK task1, task2, task3;
RT_SEM sem;
RT_MUTEX mut;

void semWait1();
void semWait2();
void semWait3();
void busy_wait_ms(unsigned long delay);

int main(){
	rt_print_auto_init(1);
	mlockall(MCL_CURRENT|MCL_FUTURE);
	rt_task_shadow(NULL, "main", 5, T_CPU(0)|T_JOINABLE);
	#ifdef semaphore 
	rt_sem_create(&sem, "semaphore", 1, 0);
	#endif
	#ifdef mutex
	rt_mutex_create(&mut, "mutex");
	#endif

	rt_task_create(&task1, "Task1", 0, 1, T_CPU(0)|T_JOINABLE);
	rt_task_create(&task2, "Task2", 0, 2, T_CPU(0)|T_JOINABLE);
	rt_task_create(&task3, "Task3", 0, 3, T_CPU(0)|T_JOINABLE);
	
	rt_task_start(&task1, &semWait1, NULL);
	rt_task_start(&task2, &semWait2, NULL);
	rt_task_start(&task3, &semWait3, NULL);

	rt_printf("sync \n");
	rt_task_sleep(100000000);
	rt_sem_broadcast(&sem);
	rt_task_sleep(100000000);



	rt_task_join(&task1);
	rt_task_join(&task2);
	rt_task_join(&task3);
	#ifdef semaphore
	rt_sem_delete(&sem);
	#endif
	#ifdef mutex
	rt_mutex_delete(&mut);
	#endif
}

void semWait1(){
	#ifdef semaphore
	rt_sem_p(&sem, 0);
	#endif
	#ifdef mutex
	rt_mutex_acquire(&mut, 0);
	#endif

	rt_printf("Low pri task: start\n");
	//rt_timer_spin(3000000);
	busy_wait_ms(3);
	rt_printf("Low pri task: unlocks sem\n");
	#ifdef semaphore
	rt_sem_v(&sem);
	#endif
	#ifdef mutex
	rt_mutex_release(&mut);
	#endif
	rt_printf("Low pri task: stop \n");
}

void semWait2(){
	rt_task_sleep(1000000);
	rt_printf("Medium pri task: start\n");
	//rt_timer_spin(5000000);
	busy_wait_ms(5);
	rt_printf("Medium pri task: stop \n");
}

void semWait3(){
	rt_task_sleep(2000000);
	#ifdef semaphore
	rt_sem_p(&sem, 0);
	#endif
	#ifdef mutex
	rt_mutex_acquire(&mut, 0);
	#endif
	rt_printf("High pri task: start \n");
	//rt_timer_spin(2000000);
	busy_wait_ms(2);
	#ifdef semaphore
	rt_sem_v(&sem);	
	#endif
	#ifdef mutex
	rt_mutex_release(&mut);
	#endif
	rt_printf("High pri task: stop \n");
}

void busy_wait_ms(unsigned long delay){
	unsigned long count = 0;
	while (count <= delay*10){
		rt_timer_spin(1000*100);
		count++;
	}
}