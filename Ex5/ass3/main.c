
#include <unistd.h>	//usleep
#include <stdlib.h>	//malloc, free
#include <pthread.h>
#include "sched.h"
#include "io.h"


#define N_USEFULL_THREADS	3
#define N_RETARDED_THREADS	10
#define SLEEP_PERIOD		100000

static void *usefull_thread_function(void *argpointer);
static void *retarded_thread_function(void *argpointer);
void timespec_add_us(struct timespec *t, long us);

int set_cpu(int cpu_number);

int main(void){
	
	/* init hardware */
	int ret=io_init();
	if(ret < 1){
		printf("io init failed");
		return 1;
	} 
	
	/* init usefull threads */
	int check;
	int* chan;
	pthread_t threads_usefull[N_USEFULL_THREADS];
	for(int i=0; i<N_USEFULL_THREADS; i++){
		/* Give the new thread the channel to respond to by argument */
		chan = malloc(sizeof(int));
		*chan = i+1;
		//printf("chan malloca. val: %i, adr: %i \n", *chan, chan);
		
		/* Create new thread */
		check = pthread_create(&threads_usefull[i], NULL, usefull_thread_function, (void*) chan);
		if(check == 0){
			printf("Usefull Thread %i created successfully!\n", i);
		}
		else{
			printf("Failed to create usefull thread %i\n", i);
			exit(1);
		}
	}
	/* init retarded threads */
	pthread_t threads_retarded[N_RETARDED_THREADS];
	for(int i=0; i<N_RETARDED_THREADS; i++){
		/* Create new thread */
		check = pthread_create(&threads_retarded[i], NULL, retarded_thread_function, NULL);
		if(check == 0){
			printf("Retard Thread %i created successfully!\n", i);
		}
		else{
			printf("Retard Failed to create thread %i\n", i);
			exit(1);
		}
	}

	/* Collect threads before exiting */
	for(int j = 0; j < N_USEFULL_THREADS; j++){
		pthread_join(threads_usefull[j], NULL);
	}
	
	return 0;
}

/* Polls the signal line given by argument and reacts on the corresponding line */
static void *usefull_thread_function(void *argpointer){
	set_cpu(1);
	int chan = *((int *) argpointer);
	free(argpointer);

	struct timespec next;
	clock_gettime(CLOCK_REALTIME, &next);

	while(1){
		timespec_add_us(&next, SLEEP_PERIOD);

		if(io_read(chan) == 0){
			io_write(chan, 0);
			usleep(5);
			io_write(chan, 1);
		}
		clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next, NULL);
	}	
}


static void *retarded_thread_function(void *argpointer){
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

void timespec_add_us(struct timespec *t, long us)
{
	/* add microseconds to timespecs nanosecond counter */
	t->tv_nsec += us;	// was: t->tv_nsec += us*1000;

	/* if wrapping nanosecond counter, increment second counter */
	if (t->tv_nsec > 1000000000){
		t->tv_nsec = t->tv_nsec - 1000000000;
		t->tv_sec += 1;
	}
}


