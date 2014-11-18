#ifndef _THREAD_SOCKET_H_
#define _THREAD_SOCKET_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "shared.h"

void *thread_socket(void *argpointer);
float get_stystem_output(char* buffer, int buffer_size);

#endif /* _THREAD_SOCKET_H_ */
