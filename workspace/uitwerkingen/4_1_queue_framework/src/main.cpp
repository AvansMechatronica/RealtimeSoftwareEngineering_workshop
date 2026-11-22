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
// FreeRTOS task handle
//
// The handle is filled by xTaskCreate() when TaskADC is started. It can be
// used later to identify, suspend, resume, or delete the task. Initializing it
// to NULL makes it clear that the task does not exist before setup starts it.

///////////////////////////////////////////////////////////////////////////////
// user task handler declarations

xTaskHandle handle_TaskProducer		= NULL;
xTaskHandle handle_TaskConsumer		= NULL;
xTaskHandle handle_TaskMonitor		= NULL;

QueueHandle_t handle_Queue = NULL;

///////////////////////////////////////////////////////////////////////////////
// function prototypes
void StartUserTasks(void);
void TaskProducer(void *pvParameters);
void TaskConsumer(void *pvParameters);
void TaskMonitor(void *pvParameters);

///////////////////////////////////////////////////////////////////////////////
// HAL includes for RTSW board
#include "spi_lib.h"
#include "adc_3208_lib.h"

///////////////////////////////////////////////////////////////////////////////
// SPI and ADC instances
//
// These objects represent the SPI interface and the ADC hardware. They are
// used throughout the application to perform analog-to-digital conversions.
spi_device spi;
adc3208 adc;

///////////////////////////////////////////////////////////////////////////////
// StartUserTasks
//
// Creates the application's user task. The task runs independently from the
// Arduino loop() function under the control of the FreeRTOS scheduler.
//
// configMINIMAL_STACK_SIZE is sufficient for this small task because it only uses
// a few integer variables, and the output functions. The
// priority is deliberately kept low so system and console tasks can continue
// to run when they need processor time.

void StartUserTasks(void)
{
	BaseType_t result = pdFAIL;
	uint8_t priority = 0;
	
	uint8_t queueSize = 5;
	handle_Queue = xQueueCreate(queueSize, sizeof(double));
		
	result = xTaskCreate(TaskProducer, "tsk_Prod", (configMINIMAL_STACK_SIZE), NULL, priority, &handle_TaskProducer);
	if (result == pdPASS )
	{
	}

	priority = 0;
	result = xTaskCreate(TaskConsumer, "tsk_Cons", (configMINIMAL_STACK_SIZE), NULL, priority, &handle_TaskConsumer);
	if (result == pdPASS )
	{
	}

	priority = 0;
	result = xTaskCreate(TaskMonitor, "tsk_Mon", (configMINIMAL_STACK_SIZE), NULL, priority, &handle_TaskMonitor);
	if (result == pdPASS )
	{
	}

}

///////////////////////////////////////////////////////////////////////////////
// void TaskProducer(void *pvParameters)

void TaskProducer(void *pvParameters)
{
	double adcData = 0;
	uint32_t queueFreeSpace = 0;
	uint8_t channel = 0; // ADC channel to read from
	
	ts_printf("> Task Producer started\n");

	// TODO: enable ADC

	while (true)
	{
		adcData = adc.ReadVoltage(channel);
		queueFreeSpace = uxQueueSpacesAvailable(handle_Queue);
		if (queueFreeSpace > 0)
		{
			xQueueSend(handle_Queue, &adcData, portMAX_DELAY);
		}
		vTaskDelay(pdMS_TO_TICKS(500));
	}
	
	// we never get here!
	vTaskDelete(NULL);
}


///////////////////////////////////////////////////////////////////////////////
// void TaskConsumer(void *pvParameters)

void TaskConsumer(void *pvParameters)
{
	double dataFromQueue = 0;
	uint8_t	 ix  = 0;
	double avg = 0;
	uint32_t requiredNrOfSamples = 5;
	uint8_t itemsInQueue = 0;
	
	ts_printf("> Task Consumer started\n");

	while (true)
	{
		do 
		{
			itemsInQueue = uxQueueMessagesWaiting(handle_Queue);
		} while (itemsInQueue != requiredNrOfSamples);
		
		avg = 0;
		
		for (ix = 0; ix < requiredNrOfSamples; ix++)
		{
			xQueueReceive(handle_Queue, &dataFromQueue, portMAX_DELAY);
			avg = avg + dataFromQueue;
		}
		avg = avg / requiredNrOfSamples;
		
		ts_printf("> samples = %2d, ADC average = %4.2f\n", requiredNrOfSamples, avg);
	}
	
	// we never get here!
	vTaskDelete(NULL);
}


///////////////////////////////////////////////////////////////////////////////
// void UserTask_2(void *pvParameters)

void TaskMonitor(void *pvParameters)
{
	uint8_t itemsInQueue = 0;
	
	ts_printf("> Task Monitor started\n");

	while (true)
	{
		itemsInQueue = uxQueueMessagesWaiting(handle_Queue);
		
		ts_printf("> %2d items in queue\n", itemsInQueue);
		vTaskDelay(pdMS_TO_TICKS(200));
	}
	
	// we never get here!
	vTaskDelete(NULL);
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
	// observed independently from the LED controlled by TaskADC.
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
