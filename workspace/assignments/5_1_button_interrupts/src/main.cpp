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
#include "dio_lib.h"

///////////////////////////////////////////////////////////////////////////////
// FreeRTOS task handle
//
// The handle is filled by xTaskCreate() when UserTask is started. It can be
// used later to identify, suspend, resume, or delete the task. Initializing it
// to NULL makes it clear that the task does not exist before setup starts it.

xTaskHandle handle_TaskInterruptHandler	= NULL;

///////////////////////////////////////////////////////////////////////////////
// Function prototypes
//
// These declarations make the task and startup functions available before
// their implementations below. FreeRTOS tasks use a void pointer parameter,
// even when the task does not need application-specific parameters.

void TaskInterruptHandler(void *pvParameters);
void StartUserTasks(void);


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

void StartUserTasks(void)
{
	BaseType_t result = pdFAIL;
	uint8_t priority = 0;
	
	result = xTaskCreate(TaskInterruptHandler, "tsk_IH", (configMINIMAL_STACK_SIZE), NULL, priority, &handle_TaskInterruptHandler);
	if (result == pdPASS )
	{
		// The task was created successfully. The scheduler will call UserTask
		// when the task receives processor time.
	}
	// When creation fails, the task handle remains NULL. There is currently no
	// recovery action here, so the application continues without UserTask.
}

// TODO: declareer variabele g_InterruptCount met beginwaarde 0

// TODO: declareer semafoorhandle handle_InterruptSemaphore met beginwaarde NULL

///////////////////////////////////////////////////////////////////////////////
// void interruptHandler(uint32_t id, uint32_t mask)
//
// Dit is de Interrupt Service Routine (ISR) - maak deze zo kort mogelijk!
//
// ***********************************************************************
// Deze functie wordt NIET aangeroepen vanuit het programma, maar wordt
// door de CPU "aangeroepen" op het moment dat er een interrupt optreedt.
// Deze functie wordt gekoppeld aan een interruptbron m.b.v. de functie
// interrupt_AttachHandler
// ***********************************************************************

void interruptHandler(uint32_t id, uint32_t mask)
{
	// de parameters id en mask worden niet gebruikt
	
	// TODO: volgende deel afmaken:
	
	/*
	if (<je eigen semafoor handle> != NULL)
	{
		// TODO: verhoog interrupt count
		// TODO: deblokkeer TaskInterruptHandler met semafoor
	}
	*/
}

///////////////////////////////////////////////////////////////////////////////
// void TaskInterruptHandler(void *pvParameters)
//
// Deze task is de deferred interrupt handler

void TaskInterruptHandler(void *pvParameters)
{
	ts_printf("> Task InterruptHandler started\n");
	
	// TODO: attach interrupt handler voor drukknoppen:
	//		juiste pins
	//		juiste edge
	// Gebruik de functie interrupt_AttachHandler
	
	// TODO: enable CPU interrupts
	// gebruik de functie interrupt_Enable
	
	// TODO: maak counting semafoor met juiste beginwaarden
	
	while (true)
	{
		// TODO: wacht op semafoor, deze wordt gedeblokkeerd in de ISR
		
		ts_printf("> Interrupt!\n");
		
		// TODO: laat waarde zien van globale variabele die de interrupts telt
	}
	
	// we never get here!
	vTaskDelete(NULL);
}




///////////////////////////////////////////////////////////////////////////////
// UserTask
//
// Initializes the digital I/O abstraction and repeatedly toggles one output
// bit. The connected LED therefore changes state on every cycle, providing a
// simple visual indication that the task is alive and being scheduled.
//
// pvParameters is part of the standard FreeRTOS task signature. This task does
// not currently require parameters, so it is intentionally unused.

void UserTask(void *pvParameters)
{
	dio_device dio;
	// Initialize the DIO hardware before accessing the digital output, LED's are connected to it.
	dio.Init();

	// Report that the task has started. This message uses the normal output
	// function and is therefore independent of the DEBUG build switch.
	ts_printf("> UserTask started\n");


	
	while (true)
	{

	}
	
	// The loop is intentionally infinite, so this line is normally unreachable.
	// It remains as the correct cleanup operation if the loop is later changed
	// to terminate or the task receives an explicit exit condition.
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
