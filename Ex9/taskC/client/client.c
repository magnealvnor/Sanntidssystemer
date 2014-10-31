#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include "fifo.h"
#define DELAY_PERIOD 3000

void read_msg(char* buffer, int buffersize, int should_block);

int main(int argc, char *argv[]) {

	char buffer[WIDTH];

	while(1){
		read_msg(buffer, WIDTH, 1);
		printf("%s\n", buffer);
		memset(buffer, '\0', WIDTH);

		delay(DELAY_PERIOD);
	}

	return EXIT_SUCCESS;
}


void read_msg(char* buffer, int buffersize, int should_block){
	int fd;
	if(should_block){
		fd = open("net/ResourceMan/dev/myresource", O_RDWR);
	}else{
		fd = open("net/ResourceMan/dev/myresource", O_RDWR | O_NONBLOCK);
	}
	read(fd, buffer, buffersize);

	close(fd);
}
