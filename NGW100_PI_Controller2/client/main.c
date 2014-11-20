#include "main.h"

#define N_THREADS 3
#define SEM_SIGNAL_INIT_VAL 0
#define SEM_PID_INIT_VAL	0


int main(int argc, char** argv){

#if SIGNAL_HANDLING
	/* Init semaphore for signaling between socket and signal thread */
  	if(sem_init(&shared_signal,0,SEM_SIGNAL_INIT_VAL) == SUCCESS){
		printf("Signal Semaphore created successfully!\n");
	}else{
		printf("Failed to create signal semaphore\n");
		exit(1);
	}
#endif /* SIGNAL_HANDLING */

	/* Init semaphore for signaling between socket and pid thread */
  	if(sem_init(&shared_pid,0,SEM_PID_INIT_VAL) == SUCCESS){
		printf("pid Semaphore created successfully!\n");
	}else{
		printf("Failed to create pid semaphore\n");
		exit(1);
	}

	/* The system output y needs to be shared between socket and pid thread, thus a mutex is needed */
	shared_system_output_t y;
	y.value = 0;
	if (pthread_mutex_init(&y.lock, NULL) != 0)
    {
        printf("Mutex init failed\n");
        return 1;
    }

    /* Create UDP socket */
    if(udp_init_client(&(shared_server_con.socket), SERVER_PORT, CLIENT_IP) == FAIL){
		printf("ERROR: Socket init failed.\n");
		exit(1);
	}else{
		printf("Socket inited.\n");
	}

	/* Init Socket listener thread */
	pthread_t handle_socket;
	if(pthread_create(&handle_socket, NULL, thread_socket, (void*) &y) == SUCCESS){
		printf("Socket Thread created successfully!\n");
	}else{
		printf("Failed to create socket thread\n");
		exit(1);
	}

#if SIGNAL_HANDLING

	/* Init Signal handling thread */
	pthread_t handle_signal;
	if(pthread_create(&handle_signal, NULL, thread_signal, NULL) == SUCCESS){
		printf("Signal Thread created successfully!\n");
	}else{
		printf("Failed to create signal thread\n");
		exit(1);
	}
	
#endif /* SIGNAL_HANDLING */

	/* Init PID controller thread */
	pthread_t handle_pid;
	if(pthread_create(&handle_pid, NULL, thread_pid, (void*) &y) == SUCCESS){
		printf("PID Thread created successfully!\n");
	}else{
		printf("Failed to create PID thread\n");
		exit(1);
	}

	/* Start the system simulation */
	if(udp_send(&shared_server_con.socket, "START", sizeof("START")) == FAIL){
		printf("ERROR: Failed to send start msg.\n");
		return -1;
	}else{
		printf("Sent: START.\n");
	}

	/* Wait for threads to terminate */
	pthread_join(handle_pid, NULL);
	
	/* Stop the system simulation */
	if(udp_send(&shared_server_con.socket, "STOP", sizeof("STOP")) == FAIL){
		printf("ERROR: Failed to send stop msg.\n");
		return -1;
	}else{
		printf("Sent: STOP.\n");
	}

	return 0;
}
