#ifndef _SHARED_H_
#define _SHARED_H_ 

#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "miniproject.h"

/* Function returnv alues */
#define SUCCESS	0
#define FAIL 	(-1)

/* Constants needed for socket programming */
#define CLIENT_IP "192.168.0.1"
#define SERVER_PORT	9999
#define BUFFER_SIZE	255

typedef struct system_output{
	float value;
	pthread_mutex_t  lock;
} shared_system_output_t;

typedef struct server_con{
	struct udp_conn socket;
	pthread_mutex_t lock;
} shared_server_con_t;

/* Semaphores are used for signaling between threads*/
sem_t shared_signal, shared_pid;

pthread_barrier_t shared_start;

shared_server_con_t shared_server_con;

#endif /* _SHARED_H_ */
