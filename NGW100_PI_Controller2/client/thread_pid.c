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
		.previous_error 	= 1.0,
		.integral	= 0.0
	};

	struct timespec period_timer;
	clock_gettime(CLOCK_REALTIME, &period_timer);

	int counter = 0;

	/* Wait untill all threads are ready */
    //pthread_barrier_wait(&shared_start);

	while(1){
		counter++;
		timespec_add_us(&period_timer, PID_PERIOD);
		//printf("\tpid_awake\n");

		/* Request system Output from server */
		if(udp_send(&shared_server_con.socket, "GET", sizeof("GET")) == FAIL){
			printf("\tERROR: send failed.\n");
			exit(1);
		}else{
		//	printf("\tSent: GET \n");
		}

		/* Wait for new value */
		//printf("\tpid awaiting new y\n");
		sem_wait(&shared_pid);
		//printf("\tPid got new y\n");
		pthread_mutex_lock(&(y->lock));
		float system_output = y->value;
		pthread_mutex_unlock(&(y->lock));

		//printf("\ty=%f\n", system_output);

		/* Calculate the corresponding output */
		controller_output = get_controller_output(system_output, &Controller);
		sprintf(buffer, "SET:%f", controller_output);
		//printf("\tController_output is: %f\n", controller_output);

		/* Request the UDP socket */
		//printf("\tpid requesting socket\n");
		pthread_mutex_lock(&shared_server_con.lock);
		//printf("\tpid granted socket\n");

		/* Send the calculated controller output to the system on server */
		if(udp_send(&shared_server_con.socket, buffer, sizeof(buffer)) == FAIL){
			printf("\tERROR: send failed.\n");
			exit(1);
		}else{
		//	printf("\tSent: %s \n", buffer);
		}

		/* Reslease the UDP socket */
		pthread_mutex_unlock(&shared_server_con.lock);

		if(counter*PID_PERIOD > RUNTIME) {
		//if(counter > 4) {
			break;
		}

		//printf("\tpid sleeping\n");
		/* Sleep until next period */
		clock_nanosleep(&period_timer);
	}
}

float get_controller_output(float system_output, PID_ctrl_t *Controller){
	float error, derivative;

	/* Calculate the different contributions */
	error = Controller->reference - system_output;
	Controller->integral = Controller->integral + (error * Controller->period);
	derivative = (error - Controller->previous_error);

	/* Store the error to be used in next calculation */
	Controller->previous_error = error;

	/*
	printf("\tPID_test: \n\terror: %f\n\tintegral: %f\n\tderivative: %f\n\tKp: %f\n\tKi: %f\n\tKd: %f\n", 
			error, 
			Controller->integral, 
			derivative,
			Controller->Kp,
			Controller->Ki,
			Controller->Kd);
	*/

	/* Return the  controller output */
	return Controller->Kp*error + Controller->Ki*Controller->integral + Controller->Kd*derivative;
}
