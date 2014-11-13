#ifndef CLIENT_PID_CONTROLLER_H_
#define CLIENT_PID_CONTROLLER_H_

/* PID Controller variables */
#define PID_KP			13.32
#define PID_KI			1776.0
#define PID_KD			0.025
#define PID_REFERENCE	1.0
#define PID_PERIOD		1000
#define RUNTIME			500000000

typedef struct{
	float Kp;
	float Ki;
	float Kd;
	float reference;
	float previous_error;
	int period;
}PID_ctrl_t;

float get_controller_output(float system_output,PID_ctrl_t *Controller);
float get_stystem_output(char* buffer, int buffer_size);

#endif /* MINIPROJECT_H_ */
