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

void error(char *s){
	perror(s);
	exit(EXIT_FAILURE);
}

fifo_t Queues;

char buffer[WIDTH];

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

	/* Init queue for holding blocked threads aswell as messages */
	init_fifo(&Queues);

	/* Wait forever, handling messages. */
	ctp = dispatch_context_alloc(dpp);
	while(1){
		if (!(ctp = dispatch_block(ctp)))
			error("dispatch_block()");
		dispatch_handler(ctp);
	}

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

    /* Add to queue, if there is a waiting client, start it */
    pthread_mutex_lock(&Queues.resource_mutex);
    if(Queues.blockedHead != Queues.blockedTail){
        MsgReply(fifo_rem_blocked_id(&Queues), 0, buf, WIDTH);
    }else{
    	fifo_add_string(&Queues ,buf);
    }
    pthread_mutex_unlock(&Queues.resource_mutex);

    free(buf);

    return (_RESMGR_NPARTS (0));
}

int io_read (resmgr_context_t *ctp, io_read_t *msg, RESMGR_OCB_T *ocb){
    int nbytes;
    int buffer_read;
    int nonblock;

    /* Check if the buffer has been read */
    if(ocb->offset){
    	nbytes = 0;
    	_IO_SET_READ_NBYTES (ctp, 0);
    	buffer_read = 0;
    }else{
    	nbytes = WIDTH;

    	/* Check if queue is empty */
    	pthread_mutex_lock(&Queues.resource_mutex);
    	if(Queues.fifoTail == Queues.fifoHead){

    		/* Check if call should be blocking or nonblocking */
    		iofunc_read_verify (ctp, msg, ocb, &nonblock);

    		if(!nonblock){
    			fifo_add_blocked_id(&Queues, ctp->rcvid);
    			return (_RESMGR_NOREPLY);
    		}else{
    			return EAGAIN;
    		}
    	}else{
    		fifo_rem_string(&Queues, buffer);
    		printf("%s\n", buffer);
    	}
    	pthread_mutex_unlock(&Queues.resource_mutex);

		/* Set up the return data IOV */
		SETIOV (ctp->iov, buffer, nbytes);

		/* Set up the number of bytes (returned by client's read()) */
		_IO_SET_READ_NBYTES (ctp, nbytes);

		/* Set the offset to one, indicating the buffer has been read */
		ocb->offset = 1;
		buffer_read = 1;
    }
    return (_RESMGR_NPARTS(buffer_read));
}

