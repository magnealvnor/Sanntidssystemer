#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <string.h>
#include <pthread.h>

#include "fifo.h"

#define COUNTER_PERIOD 	1000
#define COUNTER_STEP	1
#define DIR_UP			1
#define DIR_NONE		0
#define DIR_DOWN		(-1)
#define BUFFER_SIZE		1

void error(char *s){
	perror(s);
	exit(EXIT_FAILURE);
}

int32_t counter_var = 0;
int32_t counter_dir = 0;

pthread_mutex_t lock;
pthread_t tid;
char buffer[BUFFER_SIZE];

static void* counter(void *arg);
int io_read (resmgr_context_t *ctp, io_read_t *msg, RESMGR_OCB_T *ocb);
int io_write (resmgr_context_t *ctp, io_write_t *msg, RESMGR_OCB_T *ocb);

int main(int argc, char *argv[]) {
	dispatch_t *dpp;
	resmgr_attr_t resmgr_attr;
	dispatch_context_t *ctp;
	resmgr_connect_funcs_t connect_funcs;
	resmgr_io_funcs_t io_funcs;
	iofunc_attr_t io_attr;

	printf("Start resource manager\n");

	/* Init mutex for counter variable */
	if (pthread_mutex_init(&lock, NULL) != 0){
	        printf("\n mutex init failed\n");
	        return 1;
	}

	/* Create dispatch. */
	if (!(dpp = dispatch_create()))
		error("dispatch_create()");

	/* Initialize resource manager attributes. */
	memset(&resmgr_attr, 0, sizeof(resmgr_attr));
	resmgr_attr.nparts_max = 1;
	resmgr_attr.msg_max_size = 2048;

	/* Set standard connect and io functions. */
	iofunc_func_init(_RESMGR_CONNECT_NFUNCS,
				&connect_funcs,
				_RESMGR_IO_NFUNCS,
				&io_funcs);
	iofunc_attr_init(&io_attr, S_IFNAM | 0666, 0, 0);

	/* Override functions */
	io_funcs.read = io_read;
	io_funcs.write = io_write;

	/* Establish resource manager */
	if (resmgr_attach(dpp,
				&resmgr_attr,
				"/dev/myresource",
				_FTYPE_ANY,
				0,
				&connect_funcs,
				&io_funcs,
				&io_attr)
					< 0)
		error("resmgr_attach()");

	/* Create counter thread */
	if (pthread_create(&tid, NULL, &counter, NULL) != 0)
		printf("can't create counter thread\n");

	/* Wait forever, handling messages. */
	ctp = dispatch_context_alloc(dpp);
	while(1){
		if (!(ctp = dispatch_block(ctp)))
			error("dispatch_block()");
		dispatch_handler(ctp);
	}
	pthread_mutex_destroy(&lock);

	exit(EXIT_SUCCESS);
}

int io_write (resmgr_context_t *ctp, io_write_t *msg, RESMGR_OCB_T *ocb){
    char    *buf;

    /* set up the number of bytes (returned by client's write()) */
    _IO_SET_WRITE_NBYTES (ctp, msg->i.nbytes);

    buf = (char *) malloc(msg->i.nbytes + 1);
    if (buf == NULL)
        return(ENOMEM);

    /*
     *  Reread the data from the sender's message buffer.
     *  We're not assuming that all of the data fit into the
     *  resource manager library's receive buffer.
     */

    resmgr_msgread(ctp, buf, msg->i.nbytes, sizeof(msg->i));
    buf [msg->i.nbytes] = '\0'; /* just in case the text is not NULL terminated */
    printf ("Received %d bytes = '%s'\n", msg -> i.nbytes, buf);

    /* Set the dir var for counter thread according to the command */
    if(!strncmp(buf, "up", 2)){
    	/* Count upwards */
    	pthread_mutex_lock(&lock);
    	counter_dir = DIR_UP;
    	pthread_mutex_unlock(&lock);

    }else if(!strncmp(buf,"down", 4)){
    	/* Count downwards */
    	pthread_mutex_lock(&lock);
    	counter_dir = DIR_DOWN;
    	pthread_mutex_unlock(&lock);

    }else if(!strncmp(buf, "stop", 4)){
    	/* Stop counting */
    	pthread_mutex_lock(&lock);
    	counter_dir = DIR_NONE;
    	pthread_mutex_unlock(&lock);

    }else{
    	// do nothing
    }

    free(buf);

    return (_RESMGR_NPARTS (0));
}

int io_read (resmgr_context_t *ctp, io_read_t *msg, RESMGR_OCB_T *ocb){
    int nbytes;
    int buffer_read;

    /* Check if the buffer has been reads */
    if(ocb->offset){
    	nbytes = 0;
    	_IO_SET_READ_NBYTES (ctp, 0);
    	buffer_read = 0;
    }else{
    	nbytes = BUFFER_SIZE;

    	pthread_mutex_lock(&lock);
		sprintf(buffer, "%i\n", counter_var);
		pthread_mutex_unlock(&lock);

		printf("buffer is: %s \n", buffer);

		/* Set up the return data IOV */
		SETIOV (ctp->iov, buffer, nbytes);

		/* Set up the number of bytes (returned by client's read()) */
		_IO_SET_READ_NBYTES (ctp, nbytes);

		/* Set the offset to one, indicating the buffer has been read */
		ocb->offset = 1;
		buffer_read = 1;
    }
    return (_RESMGR_NPARTS (buffer_read));
}

static void* counter(void *arg){
	printf("Counter thread started!\n");

	while(1){
		pthread_mutex_lock(&lock);
		counter_var += counter_dir * COUNTER_STEP;
		pthread_mutex_unlock(&lock);
		delay(COUNTER_PERIOD);
	}
	return NULL;
}
