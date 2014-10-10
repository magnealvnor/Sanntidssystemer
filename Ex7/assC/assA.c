#include <native/sem.h>
#include <native/task.h>
#include <stdio.h>
#include <sys/mman.h>
#include <rtdk.h>
#include <native/timer.h>
#include <native/mutex.h>

//#define semaphore
#define mutex

static RT_TASK task1, task2;

RT_MUTEX a, b;

void semWait1();
void semWait2();
void busy_wait_ms(unsigned long delay);

int main(){
	rt_print_auto_init(1);
	mlockall(MCL_CURRENT|MCL_FUTURE);
	rt_task_shadow(NULL, "main", 5, T_CPU(0)|T_JOINABLE);

	#ifdef mutex
	rt_mutex_create(&a, "Mutex");
	rt_mutex_create(&b, "b");
	#endif

	rt_task_create(&task1, "Task1", 0, 1, T_CPU(0)|T_JOINABLE);
	rt_task_create(&task2, "Task2", 0, 2, T_CPU(0)|T_JOINABLE);
	
	
	rt_task_start(&task1, &semWait1, NULL);
	rt_task_start(&task2, &semWait2, NULL);

	rt_printf("sync \n");
	
	rt_task_join(&task1);
	rt_task_join(&task2);

	#ifdef mutex
	rt_mutex_delete(&a);
	rt_mutex_delete(&b);
	#endif
}

void semWait1(){
	
	rt_mutex_acquire(&a, 0);
	rt_printf("Low pri task: lock a\n");
	//rt_timer_spin(3000000);
	busy_wait_ms(3);
	rt_mutex_acquire(&b, 0);
	rt_printf("Low pri task: lock b\n");
	busy_wait_ms(3);
	
	rt_printf("Low pri task: unlock b \n");
	rt_mutex_release(&b);
	rt_printf("Low pri task: unlock a \n");
	rt_mutex_release(&a);
	
	rt_printf("Low pri task: stop \n");
}

void semWait2(){
	rt_task_sleep(1000000);
	rt_mutex_acquire(&b, 0);
	rt_printf("High pri task: lock b\n");
	busy_wait_ms(1);
	rt_mutex_acquire(&a, 0);
	rt_printf("High pri task: lock a\n");
	busy_wait_ms(2);
	rt_printf("High pri task: release a\n");
	rt_mutex_release(&a);
	rt_printf("High pri task: release b \n");
	rt_mutex_release(&b);
	busy_wait_ms(1);
	
	
}


void busy_wait_ms(unsigned long delay){
	unsigned long count = 0;
	while (count <= delay*10){
		rt_timer_spin(1000*100);
		count++;
	}
}