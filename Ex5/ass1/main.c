
#include <unistd.h>	//usleep
#include <stdlib.h>	//malloc, free
#include <pthread.h>
#include "io.h"


#define N_THREADS	3
#define DEBUG		1

static void *threadfunction(void *argpointer);

int main(void){
	
	/* init hardware */
	int ret=io_init();
	if(ret < 1){
		printf("io init failed");
		return 1;
	} 
	
	/* init threads */
	int check;
	int* chan;
	pthread_t threads[N_THREADS];
	for(int i=0; i<N_THREADS; i++){
		/* Give the new thread the channel to respond to by argument */
		chan = malloc(sizeof(int));
		*chan = i+1;
		printf("chan malloca. val: %i, adr: %i \n", *chan, chan);
		
		/* Create new thread */
		check = pthread_create(&threads[i], NULL, threadfunction, (void*) chan);
		if(check == 0){
			printf("Thread %i created successfully!\n", i);
		}
		else{
			printf("Failed to create thread %i\n", i);
			exit(1);
		}
	}

	/* Collect threads before exiting */
	for(int j = 0; j < N_THREADS; j++){
		pthread_join(threads[j], NULL);
	}
	
	return 0;
}

/* Polls the signal line given by argument and reacts on the corresponding line */
static void *threadfunction(void *argpointer){
	int chan = *((int *) argpointer);
	free(argpointer);
	printf("channel value: %i, channel adress: %i \n", chan, &chan);
	while(1){
		if(io_read(chan) == 0){
			io_write(chan, 0);
			usleep(5);
			io_write(chan, 1);
		}
	}	
}
