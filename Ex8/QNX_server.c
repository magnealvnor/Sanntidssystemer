#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/dispatch.h>
#include <sys/mman.h>

#define SERVER_PRIORITY 25

static int set_priority(int priority);
static int get_priority(void);

typedef struct{
	pthread_mutex_t pid_mutex;
	pid_t pid;
} pid_data;

int main(int argc, char *argv[]) {
	printf("Server started \n");

	/* Create shared memory */
	int file_descriptor;
	file_descriptor = shm_open("/sharedpid", O_RDWR | O_CREAT, S_IRWXU);

	/* Resize shared memory */
	ftruncate(file_descriptor, sizeof(pid_data));

	/* Map shared memory into address space */
	void* shared_memory;
	shared_memory = mmap(0, sizeof(pid_data), PROT_READ | PROT_WRITE, MAP_SHARED, file_descriptor, 0);

	/* Create struct */
	pid_data data;
	data.pid = getpid();

	/* Create Mutex */
	pthread_mutex_t lock = ((pid_data*)shared_memory)->pid_mutex;
	pthread_mutexattr_t myattr;
	pthread_mutexattr_init(&myattr);
	pthread_mutexattr_setpshared(&myattr, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(&lock, &myattr );

	/* Set pid in shared memory */
	pthread_mutex_lock(&lock);
	*((pid_data*)shared_memory) = data;
	pthread_mutex_unlock(&lock);
	printf("Pid stored in shared memory \n");


	/* Create channel */
	int ch_id = ChannelCreate(0);
	printf("Channel %d created \n", ch_id);
	int rcv_id;
	int buffer_recv;
	int buffer_resp;
	struct _msg_info msg_info;

	/* Set priority */
	set_priority(SERVER_PRIORITY);

	/* Listen for client */
	printf("Server listening... \n");
	while(1){
		printf("Server priority: %d\n", get_priority());
		rcv_id = MsgReceive(ch_id, &buffer_recv, sizeof(buffer_recv), &msg_info);
		buffer_resp = buffer_recv + 1;
		printf("Received: %d, Responding: %d \n", buffer_recv, buffer_resp);

		if(MsgReply(rcv_id, EOK, &buffer_resp, sizeof(buffer_resp)) != EOK){
			printf("ERROR: failed to reply to message: %d \n", buffer_recv);
		}
	}

	return EXIT_SUCCESS;
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
