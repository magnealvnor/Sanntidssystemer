#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/dispatch.h>
#include <sys/mman.h>

#define N_PTHREADS 4
#define SUCCESS 0
#define CLIENT_MAIN_PRIORITY 50

typedef struct{
	pthread_mutex_t pid_mutex;
	pid_t pid;
} pid_data;

typedef struct{
	int tid;
	pid_t pid;
	int priority;
}Msg_t;

static void *pthread_function(void *argpointer);
static int set_priority(int priority);
static int get_priority(void);

int main(int argc, char *argv[]) {
	printf("Client started \n");

	set_priority(CLIENT_MAIN_PRIORITY);

	/* Create shared memory */
	int file_descriptor;
	file_descriptor = shm_open("/sharedpid", O_RDWR, S_IRWXU);

	/* Map shared memory into address space */
	void* shared_memory;
	shared_memory = mmap(0,
				sizeof(pid_data),
				PROT_READ | PROT_WRITE,
				MAP_SHARED,
				file_descriptor,
				0);

	/* Get pid */
	pthread_mutex_lock(&((pid_data*)shared_memory)->pid_mutex);
	pid_t pid_local = ((pid_data*)shared_memory)->pid;
	pthread_mutex_unlock(&((pid_data*)shared_memory)->pid_mutex);
	printf("Main got pid: %i \n", pid_local);

	/* Create client threads */
	pthread_t pthreads[N_PTHREADS];
	int i;
	Msg_t *info;
	for(i=0; i<N_PTHREADS; i++){
		/* Give each thread an id as param */
		info = malloc(sizeof(Msg_t));
		info->tid = i+1;
		info->pid = pid_local;

		/* Increase the priority for each thread */
		info->priority = (i+1)*10;


		/* Create new thread */
		if(pthread_create(&pthreads[i], NULL, pthread_function, (void*)info) != SUCCESS){
			printf("ERROR: Client thread %i failed to be created \n", i);
			exit(1);
		}

		/* print success */
		printf("Pthread %i initiated successfully \n", i+1);
	}

	/* Collect threads before exiting */
	int j;
	for(j = 0; j < N_PTHREADS; j++){
		pthread_join(pthreads[j], NULL);
	}

	return EXIT_SUCCESS;
}

static void *pthread_function(void *argpointer){
	/* Get thread id */
	Msg_t info = *((Msg_t *) argpointer);
	free(argpointer);
	//printf("Thread started with tid: %d and pid: %d \n", info.tid, info.pid);

	/* Set thread priority */
	set_priority(info.priority);

	/* Connect and send on chan */
	int ch_id = ConnectAttach(0, info.pid, 1, 0, 0);
	int buffer_send = info.tid;
	int buffer_recv;
	int status = MsgSend(ch_id,
				&buffer_send,
				sizeof(buffer_send),
				&buffer_recv,
				sizeof(buffer_recv));
	if(status < 0){
		printf("ERROR: Thread &d Failed to send: %d\n", info.tid, buffer_send);
		exit(1);
	}

	printf("Thread %d Sent: %d , Received: %d\n", info.tid, buffer_send, buffer_recv);

	if (ConnectDetach(ch_id)!= EOK){
		printf("ERROR: Thread %d Failed to detach from chan %d\n", info.tid, ch_id);
		exit(1);
	}
}

static int set_priority(int priority){
	int policy;
	struct sched_param param;

	// check priority in range
	if (priority < 1 || priority > 63) return -1;

	// set priority
	pthread_getschedparam(pthread_self(), &policy, &param);
	param.sched_priority = priority;
	return pthread_setschedparam(pthread_self(), policy, &param);
}

static int get_priority(void){
	int policy;
	struct sched_param param;

	// get priority
	pthread_getschedparam(pthread_self(), &policy, &param);
	return param.sched_curpriority;
}
