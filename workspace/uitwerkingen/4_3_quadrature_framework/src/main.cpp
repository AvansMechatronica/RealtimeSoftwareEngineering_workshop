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

xTaskHandle handle_HartbeatTask		= NULL;
xTaskHandle handle_QuadratureTask	= NULL;


///////////////////////////////////////////////////////////////////////////////
// function prototypes
void StartUserTasks(void);
void TaskQuadrature(void *pvParameters);

///////////////////////////////////////////////////////////////////////////////
// HAL includes for RTSW board
#include "spi_lib.h"
#include "qc_7366_lib.h"

///////////////////////////////////////////////////////////////////////////////
// SPI and Quadrature Counter instances
//
// These objects represent the SPI interface and the quadrature counter hardware. They are
// used throughout the application to read quadrature encoder values.
spi_device spi;
qc7366 qc;

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
	// TODO: queue aanmaken met globale queue handle
		
	result = xTaskCreate(TaskQuadrature, "tsk_Quad", (configMINIMAL_STACK_SIZE), NULL, priority, &handle_QuadratureTask);
	if (result == pdPASS )
	{
	}

}

///////////////////////////////////////////////////////////////////////////////
// void TaskQuadrature(void *pvParameters)

void TaskQuadrature(void *pvParameters)
{
	// volgende regel niet wijzigen!
	uint8_t mode = MODE_QC_1 | MODE_FREERUNNING | INDEX_RESETCNTR | INDEX_ASYNC | FILTERCLOCK_DIV_2;
	uint8_t qcChannel	= 1;
	int32_t qcCount		= 0;
	uint8_t qcStatus	= 0;

	ts_printf("> Task Quadrature started\n");

	qc.WriteModeRegister(qcChannel, QC_MODE_REGISTER_1, mode);
	qc.EnableCounter(qcChannel);

	while (true)
	{
		// TODO: vul de volgende code aan:
		//		Quadrature uitlezen kanaal 0qc.
		qcCount = qc.ReadCountRegister(qcChannel);
		ts_printf("Quadrature count: %ld\n", qcCount);
		vTaskDelay(pdMS_TO_TICKS(1000));
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
	qc.Init(&spi);

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
