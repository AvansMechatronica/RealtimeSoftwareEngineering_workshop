/*
 * main.cpp
 *
 * Created: 13-11-2023 19:36:36
 * Author: Roel Smeets & Gerard Harkema
 * Revision: V2.0
 * Modified: 07-09-2026
 */ 

///////////////////////////////////////////////////////////////////////////////
// system includes

#include <Arduino.h>
#include <string.h>

///////////////////////////////////////////////////////////////////////////////
// application includes
#include "command_console.h"
#include "heartbeat.h"
#include "ts_printf.h"
#include "system_info.h"


///////////////////////////////////////////////////////////////////////////////
// HAL includes for RTSW board
#include "spi_lib.h"
#include "adc_3208_lib.h"

///////////////////////////////////////////////////////////////////////////////
// HAL includes for RTSW board
#include "dio_lib.h"

///////////////////////////////////////////////////////////////////////////////
// SPI and ADC instances
//
// These objects represent the SPI interface and the ADC hardware. They are
// used throughout the application to perform analog-to-digital conversions.
spi_device spi;
adc3208 adc;

///////////////////////////////////////////////////////////////////////////////
// FreeRTOS task handle
//
// The handle is filled by xTaskCreate() when UserTask is started. It can be
// used later to identify, suspend, resume, or delete the task. Initializing it
// to NULL makes it clear that the task does not exist before setup starts it.

xTaskHandle handle_TaskTimerInterruptHandler		= NULL;
xTaskHandle handle_TaskParameterSetting			= NULL;

///////////////////////////////////////////////////////////////////////////////
// Queue used to pass the latest potmeter value from TaskParameterSetting to
// TaskTimerInterruptHandler.

QueueHandle_t handle_ParameterQueue = NULL;

///////////////////////////////////////////////////////////////////////////////
// Function prototypes
//
// These declarations make the task and startup functions available before
// their implementations below. FreeRTOS tasks use a void pointer parameter,
// even when the task does not need application-specific parameters.

void TaskTimerInterruptHandler(void *pvParameters);
void StartUserTasks(void);
void TaskParameterSetting(void *pvParameters);

///////////////////////////////////////////////////////////////////////////////
// StartUserTasks
//
// Creates the application's user task. The task runs independently from the
// Arduino loop() function under the control of the FreeRTOS scheduler.
//
// configMINIMAL_STACK_SIZE is sufficient for this small task because it only
// uses a DIO object, a few integer variables, and the output functions. The
// priority is deliberately kept low so system and console tasks can continue
// to run when they need processor time.

///////////////////////////////////////////////////////////////////////////////
// void StartUserTasks(void)

void StartUserTasks(void)
{
	BaseType_t result = pdFAIL;
	uint8_t priority = 0;
	
	UBaseType_t parameterQueueSize = 1;

	// queue for potmeter value passing
	handle_ParameterQueue = xQueueCreate(parameterQueueSize, sizeof(double));
	if (handle_ParameterQueue == NULL)
	{
	}

	result = xTaskCreate(TaskTimerInterruptHandler, "tsk_TIH", (configMINIMAL_STACK_SIZE), NULL, priority, &handle_TaskTimerInterruptHandler);
	if (result == pdPASS )
	{
	}

	result = xTaskCreate(TaskParameterSetting, "tsk_PS", (configMINIMAL_STACK_SIZE), NULL, priority, &handle_TaskParameterSetting);
	if (result == pdPASS )
	{
	}

}



#define LED_PIN 15 // GPIO15 is directly connected to the LED on the RTSW board
#define interrupt_pin 36 // GPIO36 is connected to the interrupt pin on the RTSW board (D0), PIN 1 of DB9 connector J11
// Connect deze pin aan één van de clockuitgangen van de klokgenerator op DB9 connector J6

///////////////////////////////////////////////////////////////////////////////
// void ClockInterruptHandler(void)
//
// Dit is de Interrupt Service Routine (ISR) - maak deze zo kort mogelijk!
// Deze wordt elke milliseconde uitgevoerd (f = 1 KHz, T = 1 ms)
//
// ***********************************************************************
// Deze functie wordt NIET aangeroepen vanuit het programma, maar wordt
// door de CPU "aangeroepen" op het moment dat er een clock interrupt optreedt.
// Deze functie wordt gekoppeld aan de clock interruptbron m.b.v. de functie
// attachInterrupt
//
// ***********************************************************************

// TODO: declareer variabele g_InterruptCount met beginwaarde 0
// TODO: declareer semafoorhandle handle_InterruptSemaphore met beginwaarde NULL

uint32_t g_InterruptCount = 0;
uint32_t g_id  = 0;
uint32_t g_mask = 0;

xSemaphoreHandle InterruptSemaphore = NULL;

// The ESP32 Arduino core invokes a GPIO interrupt handler without arguments,
// so the originating pin and its bitmask are filled in here instead of being
// passed in by the hardware (unlike the old SAM3X-based InterruptLib this
// code was modeled after).
void IRAM_ATTR ClockInterruptHandler(void)
{
	BaseType_t higherPriorityTaskWoken = pdFALSE;

	if (InterruptSemaphore != NULL)
	{
		g_InterruptCount++;
		g_id = interrupt_pin;
		g_mask = digitalPinToBitMask(interrupt_pin);

		if ((g_InterruptCount % 1000) == 0)
		{
			xSemaphoreGiveFromISR(InterruptSemaphore, &higherPriorityTaskWoken);
		}
	}

	portYIELD_FROM_ISR(higherPriorityTaskWoken);
}

///////////////////////////////////////////////////////////////////////////////
// void TaskInterruptHandler(void *pvParameters)

void TaskTimerInterruptHandler(void *pvParameters)
{
	uint32_t taskInterruptCount = 0;
	TickType_t ticksToWait = 0;
	double potmeterValue = 0.0;

	ts_printf("> Task Timer InterruptHandler started\n");

	uint32_t maxSemCount = 1;
	uint32_t initialSemCount = 0;

	// Create the semaphore before the interrupt is attached, so the ISR can
	// safely give it as soon as it starts triggering.
	InterruptSemaphore = xSemaphoreCreateCounting(maxSemCount, initialSemCount);

	// The external clock generator on J6 drives interrupt_pin with a 1 kHz
	// square wave, so a FALLING edge signals every millisecond. attachInterrupt
	// both registers the handler and enables the interrupt.
	pinMode(interrupt_pin, INPUT);
	attachInterrupt(digitalPinToInterrupt(interrupt_pin), ClockInterruptHandler, FALLING);

	while (true)
	{
		xSemaphoreTake(InterruptSemaphore, portMAX_DELAY);
		taskInterruptCount++;

		ts_printf("> Task interrupt!\n");
		ts_printf("> interrupt count = %lu, taskInterruptCount = %lu\n", g_InterruptCount, taskInterruptCount);
		
		// always leave parameter value in queue! So use xQueuePeek
		ticksToWait = 0;
		xQueuePeek(handle_ParameterQueue, &potmeterValue, ticksToWait);
		ts_printf("> potmeter value read: %.1f\n", potmeterValue);

	}
	
	// we never get here!
	vTaskDelete(NULL);
}


///////////////////////////////////////////////////////////////////////////////
// void TaskParameterSetting(void *pvParameters)

void TaskParameterSetting(void *pvParameters)
{
	double adcValue = 0;
	uint32_t channel = (uint32_t)(pvParameters);
	
	double minValue = 1.0;
	double maxValue = 5.0;
	
	double currentValue  = -1.0;	// start with invalid value
	double previousValue =  0.0;
	
	double threshold = 0.1;
	
	ts_printf("> starting TaskParameterSetting\n");


	
	while(true)
	{
		adcValue = adc.ReadVoltage(channel);

		if ( fabs(adcValue - previousValue) > threshold )
		{
			xQueueOverwrite(handle_ParameterQueue, &adcValue);
			ts_printf("> value set to %.1f\n", adcValue);
			previousValue = adcValue;
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
// setup
//
// Arduino calls setup() once during boot. The initialization order is
// important: serial output is configured first, followed by the shared output
// task, the heartbeat, system-information commands, the user task, and finally
// the command console. The short delays give each subsystem time to finish its
// startup before the next subsystem begins using it.

void setup (void)
{
	Serial.begin(115200);
	delay(1000);
	Serial.println("System initializing");
	// Start the task used by ts_printf and ts_debug for serialized output.
	StartTsPrintfTask(NULL);

	// Blink the board LED as a heartbeat so the system's main activity can be
	// observed independently from the LED controlled by UserTask.
	StartHeartbeatTask(LED_PIN_ESP32_BOARD);
	delay(500);
	// Add commands that expose system information through the console.
	RegisterSystemInfoCommands();
	delay(500);

	// Initialize the SPI hardware before accessing the ADC.
	spi.Init();
	// Initialize the ADC hardware before accessing the analog input.
	adc.Init(&spi);

	// Start the application task that controls the DIO output.
	StartUserTasks();
	
	// Start the interactive command console after the shared services exist.
	StartCommandConsoleTask(NULL);

	delay(500);
}

void loop(void)
{
	// Application work is performed by FreeRTOS tasks. Arduino still calls
	// loop(), so it uses a delay to yield CPU time instead of busy-spinning.
  delay(500);
}
