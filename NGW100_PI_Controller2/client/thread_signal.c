#include "thread_signal.h"

void *thread_signal(void *argpointer){

	/* Wait untill all threads are ready */
    //pthread_barrier_wait(&shared_start);

	while(1){
		sem_wait(&shared_signal);
//		printf("\t\tSignal thread awake!\n");
	}

}
