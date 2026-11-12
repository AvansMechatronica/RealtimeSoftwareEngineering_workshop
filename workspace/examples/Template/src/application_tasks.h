/*
 * application_tasks.h
 *
 * Created: 27-11-2023 15:01:19
 *  Authors: 	Roel Smeets & Gerard Harkema
 *  Revision: V2.0
 *  Modified: 07-09-2026
 */ 


#ifndef APPLICATIONTASKS_H_
#define APPLICATIONTASKS_H_

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "hardware_config.h"

///////////////////////////////////////////////////////////////////////////////
// objects made available for external use


///////////////////////////////////////////////////////////////////////////////
// function prototypes
void StartApplicationTasks(void);
bool startMyTask(HardwareConfig *hardwareConfig);
void MyTask(void *pvParameters);

#endif /* APPLICATIONTASKS_H_ */