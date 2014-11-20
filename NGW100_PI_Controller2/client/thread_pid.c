#include "thread_pid.h"

void *thread_pid(void *argpointer){
	shared_system_output_t *y = (shared_system_output_t*) argpointer;

	float controller_output;

	/* Buffer used for sending to socket */
	char buffer[BUFFER_SIZE];

	/* Init the PID controller struct*/
	PID_ctrl_t Controller = {
		.Kp			= PID_KP,
		.Ki			= PID_KI,
		.Kd			= PID_KD,
		.reference	= PID_REFERENCE,
		.period		= PID_PERIOD,
		.previous_error 	= PID_INIT_ERROR,
		.integral	= 0.0
	};

	struct timespec period_timer;
	clock_gettime(CLOCK_REALTIME, &period_timer);

	int counter = 0;

	while(1){
		counter++;
		timespec_add_us(&period_timer, PID_PERIOD*1000000);

		/* Request system Output from server */
		if(udp_send(&shared_server_con.socket, "GET", sizeof("GET")) == FAIL){
			printf("\tERROR: send failed.\n");
			exit(1);
		}
		
		/* Wait for new value */
		sem_wait(&shared_pid);

		pthread_mutex_lock(&(y->lock));
		float system_output = y->value;
		pthread_mutex_unlock(&(y->lock));

		/* Calculate the corresponding output */
		controller_output = get_controller_output(system_output, &Controller);
		sprintf(buffer, "SET:%f", controller_output);

		/* Send the calculated controller output to the system on server */
		if(udp_send(&shared_server_con.socket, buffer, sizeof(buffer)) == FAIL){
			printf("\tERROR: send failed.\n");
			exit(1);
		}

		if(counter*PID_PERIOD > RUNTIME) {
			break;
		}

		/* Sleep until next period */
		clock_nanosleep(&period_timer);
	}
	return NULL;
}

float get_controller_output(float system_output, PID_ctrl_t *Controller){
	float error, derivative;

	/* Calculate the different contributions */
	error = Controller->reference - system_output;
	Controller->integral = Controller->integral + (error * Controller->period);
	derivative = (error - Controller->previous_error);

	/* Store the error to be used in next calculation */
	Controller->previous_error = error;

	/* Return the  controller output */
	return Controller->Kp*error + Controller->Ki*Controller->integral + Controller->Kd*derivative;
}
