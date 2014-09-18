
#include <unistd.h>	//usleep
#include <stdlib.h>	//malloc, free
#include <pthread.h>
#include "sched.h"
#include "io.h"


#define N_USEFULL_THREADS	3
#define N_RETARDED_THREADS	0
#define DEBUG			1

static void *usefull_thread_function(void *argpointer);
static void *retarded_thread_function(void *argpointer);

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
	//printf("channel value: %i, channel adress: %i \n", chan, &chan);
	while(1){
		if(io_read(chan) == 0){
			io_write(chan, 0);
			usleep(5);
			io_write(chan, 1);
		}
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


