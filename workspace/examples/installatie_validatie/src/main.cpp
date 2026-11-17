/*
 * main.cpp
 *
 * Created: 13-11-2023 19:36:36
 * Author: Roel Smeets
 * Revision: V2.0
 * Modified: 07-09-2026
 */ 

///////////////////////////////////////////////////////////////////////////////
// system includes

#include <Arduino.h>
#include <string.h>

///////////////////////////////////////////////////////////////////////////////
// application includes



///////////////////////////////////////////////////////////////////////////////
// HAL includes for RTSW board

#include "led_lib.h"
#include "command_console.h"
#include "heartbeat.h"
#include "oled_lib.h"
#include "ts_printf.h"
#include "system_info.h"


///////////////////////////////////////////////////////////////////////////////
// file globals

static TaskHandle_t heartbeatTaskHandle = NULL;
led statusLed;
bool isLedOn = true;
#ifdef INCLUDE_OLED_DISPLAY
oledDisplay statusOled;
#endif

///////////////////////////////////////////////////////////////////////////////
// Stack overflow hook

void vApplicationStackOverflowHook(TaskHandle_t xTask, char * pcTaskName)
{
  Serial.printf("Stack overflow in task %s\n", pcTaskName);
	while (true)
	{
	    delay(50);
	}
}

///////////////////////////////////////////////////////////////////////////////
// void vApplicationMallocFailedHook(void)

void vApplicationMallocFailedHook(void)
{

  Serial.printf("Malloc failed!\n");
	while (true)
	{
	    delay(50);
	}
}




///////////////////////////////////////////////////////////////////////////////
// void setup(void)

void setup (void)
{
	Serial.begin(115200);
	delay(1000);
	Serial.println("System initializing");
	StartTsPrintfTask(NULL);

	StartHeartbeatTask(LED_PIN_ESP32_BOARD);
	delay(500);

	RegisterSystemInfoCommands();

	delay(500);

	StartCommandConsoleTask(NULL);

	delay(500);
}

void loop(void)
{
  // Do nothing, just delay to yield CPU time to other tasks
  delay(500);
}
