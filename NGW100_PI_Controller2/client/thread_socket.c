#include "thread_socket.h"

void *thread_socket(void *argpointer){
	/* Get parameter */
	shared_system_output_t *y = (shared_system_output_t*) argpointer;

	/* Buffer used for sending and recieving from sockets */
	char buffer[BUFFER_SIZE];

	/* Create corresponding UDP lock */
	if (pthread_mutex_init(&shared_server_con.lock, NULL) != 0)
    {
        printf("Mutex init failed\n");
        exit(1);
    }
    /* Wait untill all threads are ready */
    //pthread_barrier_wait(&shared_start);

	while(1){
	    /* Wait for answer and store it in buffer */
		if(udp_receive(&shared_server_con.socket, buffer, BUFFER_SIZE) == FAIL){
			printf("ERROR: recv failed.\n");
			exit(1);
		}else{
		//	printf("Listener recieved: %s \n", buffer);
		}

		/* Check what kind of message we've received */
		if(!strcmp(buffer, "SIGNAL")){
			printf("Waking up Signal thread!\n");
			/* Wake the signal response thread up*/
			sem_post(&shared_signal);
		}else{	//TODO:: This should check for valid input...
			/* Store the system output value in shared memory */
			pthread_mutex_lock(&(y->lock));
			y->value = get_stystem_output(buffer, BUFFER_SIZE);
			pthread_mutex_unlock(&(y->lock));

		//	printf("Waking up PID thread!\n");

			/* Wake the pid-thread up */
			sem_post(&shared_pid);
		}
	}
}

float get_stystem_output(char* buffer, int buffer_size){
//	printf("buffer: %s\n", buffer);
	float f;
	sscanf(buffer, "GET_ACK:%f", &f);
//	printf("return value: %f\n", f);
	return f;
}
