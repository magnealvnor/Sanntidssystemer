/*This file has been prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 *
 * \brief FreeRTOS Real Time Kernel example.
 *
 * Creates all the demo application tasks, then starts the scheduler.  The WEB
 * documentation provides more details of the demo application tasks.
 *
 * Main. c also creates a task called "Check".  This only executes every three
 * seconds but has the highest priority so is guaranteed to get processor time.
 * Its main function is to check that all the other tasks are still operational.
 * Each task that does not flash an LED maintains a unique count that is
 * incremented each time the task successfully completes its function.  Should
 * any error occur within such a task the count is permanently halted.  The
 * check task inspects the count of each task to ensure it has changed since
 * the last time the check task executed.  If all the count variables have
 * changed all the tasks are still executing error free, and the check task
 * toggles an LED.  Should any task contain an error at any time the LED toggle
 * will stop.
 *
 * The LED flash and communications test tasks do not maintain a count.
 *
 * - Compiler:           IAR EWAVR32 and GNU GCC for AVR32
 * - Supported devices:  All AVR32 devices with GPIO.
 * - AppNote:
 *
 * \author               Atmel Corporation: http://www.atmel.com \n
 *                       Support and FAQ: http://support.atmel.no/
 *
 *****************************************************************************/

/*
    FreeRTOS V7.0.0 - Copyright (C) 2011 Real Time Engineers Ltd.
	

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS tutorial books are available in pdf and paperback.        *
     *    Complete, revised, and edited pdf reference manuals are also       *
     *    available.                                                         *
     *                                                                       *
     *    Purchasing FreeRTOS documentation will not only help you, by       *
     *    ensuring you get running as quickly as possible and with an        *
     *    in-depth knowledge of how to use FreeRTOS, it will also help       *
     *    the FreeRTOS project to continue with its mission of providing     *
     *    professional grade, cross platform, de facto standard solutions    *
     *    for microcontrollers - completely free of charge!                  *
     *                                                                       *
     *    >>> See http://www.FreeRTOS.org/Documentation for details. <<<     *
     *                                                                       *
     *    Thank you for using FreeRTOS, and thank you for your support!      *
     *                                                                       *
    ***************************************************************************


    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation AND MODIFIED BY the FreeRTOS exception.
    >>>NOTE<<< The modification to the GPL is included to allow you to
    distribute a combined work that includes FreeRTOS without being obliged to
    provide the source code for proprietary components outside of the FreeRTOS
    kernel.  FreeRTOS is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
    more details. You should have received a copy of the GNU General Public
    License and the FreeRTOS license exception along with FreeRTOS; if not it
    can be viewed here: http://www.freertos.org/a00114.html and also obtained
    by writing to Richard Barry, contact details for whom are available on the
    FreeRTOS WEB site.

    1 tab == 4 spaces!

    http://www.FreeRTOS.org - Documentation, latest information, license and
    contact details.

    http://www.SafeRTOS.com - A version that is certified for use in safety
    critical systems.

    http://www.OpenRTOS.com - Commercial support, development, porting,
    licensing and training services.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <intc.h>
#include <interrupt.h>
#include <sleepmgr.h>
#include <uc3/sleepmgr.h>
#include <gpio.h>
#include <flashc.h>
#include <power_clocks_lib.h>
#include <sleep.h>
#include <sysclk.h>
#include <tc.h>
#include <usart.h>
#include <udi_cdc.h>
#include <usb_protocol_cdc.h>
#include <udc.h>
#include <udd.h>
#include <compiler.h>
#include <status_codes.h>
#include <board.h>
#include <stdio_usb.h>


// defines for USB UART
#define CONFIG_USART_IF (AVR32_USART2)

// include FreeRTOS header files
#include "FreeRTOS.h"
#include "task.h"

// defines for BRTT interface
#define TEST_A AVR32_PIN_PA31
#define RESPONSE_A AVR32_PIN_PA30
#define TEST_B AVR32_PIN_PA29
#define RESPONSE_B AVR32_PIN_PA28
#define TEST_C AVR32_PIN_PA27
#define RESPONSE_C AVR32_PIN_PB00

void init()
{
	// board init
	board_init();
	
#if UC3L
	// clock frequencies
	#define EXAMPLE_TARGET_DFLL_FREQ_HZ     96000000  // DFLL target frequency, in Hz
	#define EXAMPLE_TARGET_MCUCLK_FREQ_HZ   12000000  // MCU clock target frequency, in Hz
	#define EXAMPLE_TARGET_PBACLK_FREQ_HZ   12000000  // PBA clock target frequency, in Hz

	// parameters to pcl_configure_clocks().
	static scif_gclk_opt_t gc_dfllif_ref_opt = { SCIF_GCCTRL_SLOWCLOCK, 0, false};
	static pcl_freq_param_t pcl_dfll_freq_param = {
		.main_clk_src = PCL_MC_DFLL0,
		.cpu_f        = EXAMPLE_TARGET_MCUCLK_FREQ_HZ,
		.pba_f        = EXAMPLE_TARGET_PBACLK_FREQ_HZ,
		.pbb_f        = EXAMPLE_TARGET_PBACLK_FREQ_HZ,
		.dfll_f       = EXAMPLE_TARGET_DFLL_FREQ_HZ,
		.pextra_params = &gc_dfllif_ref_opt
	};
	pcl_configure_clocks(&pcl_dfll_freq_param);
#else
	pcl_switch_to_osc(PCL_OSC0, FOSC0, OSC0_STARTUP);
#endif	
	
	// stdio init
	stdio_usb_init(&CONFIG_USART_IF);

	// Specify that stdout and stdin should not be buffered.

#if defined(__GNUC__) && defined(__AVR32__)
	setbuf(stdout, NULL);
	setbuf(stdin,  NULL);
#endif
}


/*********************************************************************
User decelerations
*********************************************************************/
/* Global defines */
#define RESPONSE_DELAY 1

/* TASK A  */
#define TASK_A_PRI			(tskIDLE_PRIORITY + 4)
#define TASK_A_PERIOD		5

xTaskHandle task_A;
static void vTask_A(void *pvParameters);

/* Task B */
#define TASK_B_PRI			(tskIDLE_PRIORITY + 4)
#define TASK_B_PERIOD		5

xTaskHandle task_B;
static void vTask_B(void *pvParameters);


/* Task C */
#define TASK_C_PRI			(tskIDLE_PRIORITY + 4)
#define TASK_C_PERIOD		5

xTaskHandle task_C;
static void vTask_C(void *pvParameters);

/* Task CpuWork */
#define TASK_CpuWork_PRI	(tskIDLE_PRIORITY + 1)

xTaskHandle cpuWork;
static void vCpuWork(void *pvParameters);


/* Basic Task */
static void vBasicTask(void *pvParameters);


/* Helper functions */
void init_gpio_pins_assB(void);


/*********************************************************************
Functions
*********************************************************************/
int main()
{
	// initialize
	init();
	
	// start code from here
	init_gpio_pins_assB();
	
	// start basic task
	//xTaskCreate( vBasicTask, (signed char * ) "BASIC", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
	xTaskCreate( vTask_A, (signed char *) "Task_A", configMINIMAL_STACK_SIZE, NULL, TASK_A_PRI, &task_A);
	xTaskCreate( vTask_B, (signed char *) "Task_B", configMINIMAL_STACK_SIZE, NULL, TASK_B_PRI, &task_B);
	xTaskCreate( vTask_C, (signed char *) "Task_C", configMINIMAL_STACK_SIZE, NULL, TASK_C_PRI, &task_C);
	xTaskCreate( vCpuWork, (signed char *) "CpuWork", configMINIMAL_STACK_SIZE, NULL, TASK_CpuWork_PRI, &cpuWork);

	// Start the scheduler, anything after this will not run.
	vTaskStartScheduler();
}

/*************** TASKS ********************/
static void vBasicTask(void *pvParameters)
{
	const portTickType xDelay = 1000 / portTICK_RATE_MS;
	
	while(1)
	{
		gpio_toggle_pin(LED0_GPIO);
		printf("tick\n");
		
		vTaskDelay(xDelay);
	}
}

static void vTask_A(void *pvParameters)
{
	/* init counter variable to be used for periodic execution */
	portTickType xLastWakeTime;
	const portTickType xFrequency = TASK_A_PERIOD/portTICK_RATE_MS;
	xLastWakeTime = xTaskGetTickCount();
	
	while (true)
	{
		/* Wait until the period time is up */
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
		
		/* Check for test signal */
		if (!gpio_get_pin_value(TEST_A))
		{
			gpio_set_pin_low(RESPONSE_A);
			vTaskDelay(RESPONSE_DELAY/portTICK_RATE_MS);
			gpio_set_pin_high(RESPONSE_A);
		}
	}
	
}

static void vTask_B(void *pvParameters)
{
	/* init counter variable to be used for periodic execution */
	portTickType xLastWakeTime;
	const portTickType xFrequency = TASK_B_PERIOD/portTICK_RATE_MS;
	xLastWakeTime = xTaskGetTickCount();
	
	while (true)
	{
		/* Wait until the period time is up */
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
		
		/* Check for test signal */
		if (!gpio_get_pin_value(TEST_B))
		{
			gpio_set_pin_low(RESPONSE_B);
			vTaskDelay(RESPONSE_DELAY/portTICK_RATE_MS);
			gpio_set_pin_high(RESPONSE_B);
		}
	}
	
}

static void vTask_C(void *pvParameters)
{
	/* init counter variable to be used for periodic execution */
	portTickType xLastWakeTime;
	const portTickType xFrequency = TASK_C_PERIOD/portTICK_RATE_MS;
	xLastWakeTime = xTaskGetTickCount();
	
	while (true)
	{
		/* Wait until the period time is up */
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
		
		/* Check for test signal */
		if (!gpio_get_pin_value(TEST_C))
		{
			gpio_set_pin_low(RESPONSE_C);
			vTaskDelay(RESPONSE_DELAY/portTICK_RATE_MS);
			gpio_set_pin_high(RESPONSE_C);
		}

	}
	
}

static void vCpuWork(void *pvParameters)
{
	double dummy;
	
	while(1){
		for (int i=0; i<1000000; i++)
		{
			dummy = 2.5 * i; // dummy work
		}
		gpio_toggle_pin(LED0_GPIO);
	}
	
}

/************* init *********************/
void init_gpio_pins_assB(void){
	gpio_configure_pin(TEST_A, GPIO_DIR_INPUT || GPIO_PULL_UP);
	gpio_configure_pin(TEST_B, GPIO_DIR_INPUT || GPIO_PULL_UP);
	gpio_configure_pin(TEST_C, GPIO_DIR_INPUT || GPIO_PULL_UP);
	
	gpio_configure_pin(RESPONSE_A, GPIO_DIR_OUTPUT || GPIO_INIT_HIGH);
	gpio_configure_pin(RESPONSE_B, GPIO_DIR_OUTPUT || GPIO_INIT_HIGH);
	gpio_configure_pin(RESPONSE_C, GPIO_DIR_OUTPUT || GPIO_INIT_HIGH);
}