
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "miniproject.h"
#include "client_PID_controller.h"

/* Function returnv alues */
#define SUCCESS	0
#define FAIL 	(-1)

/* Constants needed for socket programming */
#define CLIENT_IP "192.168.0.1"
#define SERVER_PORT	9999
#define BUFFER_SIZE	20


int main(int argc, char* argv[]){
	printf("Controller is online\n");

	/* Buffer used for sending and recieving from sockets */
	char buffer[BUFFER_SIZE];
	float system_output, controller_output;

	/* Init the PID controller struct*/
	PID_ctrl_t Controller = {
		.Kp			= PID_KP,
		.Ki			= PID_KI,
		.Kd			= PID_KD,
		.reference	= PID_REFERENCE,
		.period		= PID_PERIOD,
		.previous_error 	= 1.0,
	};

	printf("pid test:\n\tref: %f\n\tperiod: %d\n\terror: %f\n", 
			Controller.reference, 
			Controller.period, 
			Controller.previous_error );

	/* Init Socket to communicate with system on server */
	struct udp_conn Server_con;
	if(udp_init_client(&Server_con, SERVER_PORT, CLIENT_IP) == FAIL){
		printf("ERROR: Socket init failed.\n");
		return -1;
	}else{
		printf("Socket inited.\n");
	}

	struct timespec period_timer;
	clock_gettime(CLOCK_REALTIME, &period_timer);

	/* Start the system simulation */
	if(udp_send(&Server_con, "START", sizeof("START")) == FAIL){
		printf("ERROR: send failed.\n");
		return -1;
	}else{
		printf("Sent: START.\n");
	}

	int counter = 0;
	while(1){

		counter++;
		timespec_add_us(&period_timer, PID_PERIOD);

		/* Get System Output from server */
		if(udp_send(&Server_con, "GET", sizeof("GET")) == FAIL){
			printf("ERROR: send failed.\n");
			return -1;
		}else{
			printf("Sent: GET \n");
		}

		/* Wait for answer and store it in buffer */
		if(udp_receive(&Server_con, buffer, BUFFER_SIZE) == FAIL){
			printf("ERROR: recv failed.\n");
			return -1;
		}else{
			printf("Recieved: %s \n", buffer);
		}

		/* Get the system output as a float */
		system_output = get_stystem_output(buffer, sizeof(buffer));
		printf("System_output is: %f\n", system_output);

		/* Get controller output and put in buffer */
		controller_output = get_controller_output(system_output, &Controller);
		sprintf(buffer, "SET:%f", controller_output);
		printf("Controller_output is: %f\n", controller_output);

		/* Send the calculated controller output to the system on server */
		if(udp_send(&Server_con, buffer, sizeof(buffer)) == FAIL){
			printf("ERROR: send failed.\n");
			return -1;
		}else{
			printf("Sent: %s \n", buffer);
		}

		//if(counter*PID_PERIOD > RUNTIME) {
		if(counter > 4) {
			break;
		}
		/* Sleep until next period */
		clock_nanosleep(&period_timer);
	}

	if(udp_send(&Server_con, "STOP", sizeof("STOP")) == FAIL){
		printf("ERROR: send failed.\n");
		return -1;
	}else{
		printf("Sent: STOP.\n");
	}

	udp_close(&Server_con);

	printf("Controller is now offline\n");
	return 0;
}

float get_controller_output(float system_output, PID_ctrl_t *Controller){
	float error, integral, derivative;
	/*
	printf("pid test: \n\tref: %f \n\tperiod: %d\n\t error:%f\n", 
			Controller->reference, 
			Controller->period, 
			Controller->previous_error );
	*/

	/* Calculate the different contributions */
	error = Controller->reference - system_output;
	integral = integral + (error * Controller->period);
	derivative = (error - Controller->previous_error);

	/* Store the error to be used in next calculation */
	Controller->previous_error = error;

	printf("PID_test: \n\terror: %f\n\tintegral: %f\n\tderivative: %f\n\tKp: %f\n\tKi: %f\n\tKd: %f\n", 
			error, 
			integral, 
			derivative,
			Controller->Kp,
			Controller->Ki,
			Controller->Kd);

	/* Return the  controller output */
	return Controller->Kp*error + Controller->Ki*integral + Controller->Kd*derivative;
}

float get_stystem_output(char* buffer, int buffer_size){
	/*
	int i;
	for(i = 0; i < buffer_size; i++){
		if (buffer[i] == ':'){
			return atof(&buffer[i+1]);
		}
	}
	return (float)-1;
	*/

	float f;
	sscanf(buffer, "GET_ACK:%f", &f);
	return f;
}

