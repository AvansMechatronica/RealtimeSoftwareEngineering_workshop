/*
 * application_tasks.cpp
 *
 * Created: 27-11-2023 14:57:44
 *  Authors: 	Roel Smeets & Gerard Harkema
 *  Revision: V2.0
 *  Modified: 07-09-2026
 */ 

///////////////////////////////////////////////////////////////////////////////
// system includes

#include <Arduino.h>
#include <string.h>

///////////////////////////////////////////////////////////////////////////////
// application includes
#include "application_tasks.h"
#include "hardware_config.h"
#include "ts_printf.h"

///////////////////////////////////////////////////////////////////////////////
// application tasks handler declarations

static TaskHandle_t myTaskHandle = NULL;


///////////////////////////////////////////////////////////////////////////////
// hardware handle shared by all application tasks
HardwareConfig *hardwareConfig = NULL;



///////////////////////////////////////////////////////////////////////////////
// void StartApplicationTasks(void)

void StartApplicationTasks(void)
{

	hardwareConfig = ConfigureHardware();
	if (hardwareConfig == NULL)
	{
		ts_printf("Failed to configure hardware.\n");
		while(1);
	}

	// TODO: Start application tasks here
	startMyTask(hardwareConfig);
}

// TODO: Implement task starting functions 
bool startMyTask(HardwareConfig *hardwareConfig)
{
	BaseType_t result = pdFAIL;

	result = xTaskCreate(MyTask, "tsk_MyTask", 4 * (configMINIMAL_STACK_SIZE), hardwareConfig, 0, &myTaskHandle);
	if (result == pdPASS)
	{
		return true;
	}
	return false;
}


// TODO: Implement tasks
void MyTask(void *pvParameters)
{
	HardwareConfig *hardwareConfig = (HardwareConfig *)pvParameters;

	// Task implementation goes here

	for (;;)
	{
		// Task loop
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}	