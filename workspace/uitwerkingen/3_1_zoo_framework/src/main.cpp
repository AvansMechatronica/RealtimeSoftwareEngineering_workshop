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
// None

///////////////////////////////////////////////////////////////////////////////
// FreeRTOS task handle
//
// The handle is filled by xTaskCreate() when UserTask is started. It can be
// used later to identify, suspend, resume, or delete the task. Initializing it
// to NULL makes it clear that the task does not exist before setup starts it.

xTaskHandle handle_UserTask[4]		= {NULL, NULL, NULL, NULL};

SemaphoreHandle_t countMutex = NULL;

///////////////////////////////////////////////////////////////////////////////
// Function prototypes
//
// These declarations make the task and startup functions available before
// their implementations below. FreeRTOS tasks use a void pointer parameter,
// even when the task does not need application-specific parameters.

void UserTask(void *pvParameters);
void StartUserTasks(void);


///////////////////////////////////////////////////////////////////////////////
// StartUserTasks
//
// Creates the application's user task. The task runs independently from the
// Arduino loop() function under the control of the FreeRTOS scheduler.
//
// configMINIMAL_STACK_SIZE is sufficient for this small task because it only
// uses a few integer variables, and the output functions. The
// priority is deliberately kept low so system and console tasks can continue
// to run when they need processor time.

///////////////////////////////////////////////////////////////////////////////
// void StartUserTasks(void)

void StartUserTasks(void)
{
	BaseType_t result = pdFAIL;
	uint8_t priority = 0;
	uint32_t zooCounter = 0;
	
	countMutex = xSemaphoreCreateMutex();
	
	zooCounter = 1;
	result = xTaskCreate(UserTask,   "tsk_Counter1", (configMINIMAL_STACK_SIZE * 2), (void *)(zooCounter), priority, &handle_UserTask[0]);
	if (result == pdPASS )
	{
	}

	zooCounter = 2;
	result = xTaskCreate(UserTask,   "tsk_Counter2", (configMINIMAL_STACK_SIZE * 2), (void *)(zooCounter), priority, &handle_UserTask[1]);
	if (result == pdPASS )
	{
	}
	
	zooCounter = 3;
	result = xTaskCreate(UserTask,   "tsk_Counter3", (configMINIMAL_STACK_SIZE * 2), (void *)(zooCounter), priority, &handle_UserTask[2]);
	if (result == pdPASS )
	{
	}
	
	zooCounter = 4;
	result = xTaskCreate(UserTask,   "tsk_Counter4", (configMINIMAL_STACK_SIZE), (void *)(zooCounter), priority, &handle_UserTask[3]);
	if (result == pdPASS )
	{
	}
}


uint32_t G_NumberOfVisitors = 0;
///////////////////////////////////////////////////////////////////////////////
// UserTask
//
// Initializes the digital I/O abstraction and repeatedly toggles one output
// bit. The connected LED therefore changes state on every cycle, providing a
// simple visual indication that the task is alive and being scheduled.
//
// pvParameters is part of the standard FreeRTOS task signature. This task does
// not currently require parameters, so it is intentionally unused.
const uint32_t MaxCount = 100000;

void UserTask(void *pvParameters)
{
	uint32_t zooEntry = (uint32_t) (pvParameters);
	uint32_t ix = 0;

	ts_printf("> ZooEntry %d started\n", zooEntry);

	// TODO: tel gespecificeerd aantal bezoekers voor het entreepoortje
	for (ix = 0; ix < MaxCount; ix++)
	{
		xSemaphoreTake(countMutex, portMAX_DELAY);
		G_NumberOfVisitors = G_NumberOfVisitors + 1;
		xSemaphoreGive(countMutex);
	}

	ts_printf("> ZooEntry %lu: Total visitors = %lu\n", zooEntry, G_NumberOfVisitors);
	
	// TODO: na het tellen in een oneindinge lange sleep gaan
	while (true)
	{
		vTaskDelay(1);
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
	// observed independently from the LED controlled by UserTask.
	StartHeartbeatTask(LED_PIN_ESP32_BOARD);
	delay(500);
	// Add commands that expose system information through the console.
	RegisterSystemInfoCommands();
	delay(500);

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
