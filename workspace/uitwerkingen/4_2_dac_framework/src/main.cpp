/*
 * main.cpp
 *
 * Created: 13-11-2023 19:36:36
 * Author: Roel Smeets & Gerard Harkema
 * Revision: V2.0
 * Modified: 07-09-2026
 */ 

///////////////////////////////////////////////////////////////////////////////
// Systeem-includes

#include <Arduino.h>
#include <string.h>

///////////////////////////////////////////////////////////////////////////////
// Applicatie-includes
#include "command_console.h"
#include "heartbeat.h"
#include "ts_printf.h"
#include "system_info.h"


///////////////////////////////////////////////////////////////////////////////
// FreeRTOS task-handles
//
// Een handle wordt door xTaskCreate() ingevuld zodra de taak is aangemaakt.
// Daarmee kan de taak later worden geïdentificeerd, gepauzeerd (suspend),
// hervat (resume) of verwijderd (delete). De beginwaarde NULL geeft aan dat
// de taak nog niet bestaat zolang setup() hem niet heeft gestart.

///////////////////////////////////////////////////////////////////////////////
// Declaraties van de task-handles

xTaskHandle handle_TaskDAC = NULL;


///////////////////////////////////////////////////////////////////////////////
// Functieprototypes
void StartUserTasks(void);
void TaskDAC(void *pvParameters);


///////////////////////////////////////////////////////////////////////////////
// HAL-includes voor het RTSW-board
#include "spi_lib.h"
#include "dac_4922_lib.h"

///////////////////////////////////////////////////////////////////////////////
// SPI- en DAC-instanties
//
// Deze objecten representeren de SPI-interface en de DAC-hardware (MCP4922).
// Ze worden in de applicatie gebruikt voor digitaal-analoogconversies.
spi_device spi;
dac4922 dac;

///////////////////////////////////////////////////////////////////////////////
// StartUserTasks
//
// Maakt de applicatietaken aan. Deze taken draaien onafhankelijk van de
// Arduino-functie loop(), onder besturing van de FreeRTOS-scheduler.
//
// De stackgrootte is gebaseerd op configMINIMAL_STACK_SIZE; dat is voldoende
// voor een kleine taak met een paar lokale variabelen en tekstuitvoer. Vergroot
// de stack (bijvoorbeeld * 2) als een taak meer doet. De prioriteit is bewust
// laag gehouden, zodat systeem- en consoletaken processortijd blijven krijgen.

void StartUserTasks(void)
{
	BaseType_t result = pdFAIL;

	uint8_t priority = 1;
			
	result = xTaskCreate(TaskDAC, "tsk_DAC", (configMINIMAL_STACK_SIZE * 2), NULL, priority, &handle_TaskDAC);
	if (result == pdPASS )
	{
	}

}

///////////////////////////////////////////////////////////////////////////////
// void TaskDAC(void *pvParameters)
//
// Genereert een driehoekspanning met de DAC op kanaal 0. Sluit de BNC-uitgang
// aan op een oscilloscoop om het signaal te bekijken!
//


void TaskDAC(void *pvParameters)
{
	float dacOutputVoltage = 0.0;
	float stepSize = 0.25;
	uint8_t dacChannel = 0;
	
	ts_printf("> Task DAC started\n");
	ts_printf("> VDAC min out = %7.3f volt\n", DAC_MIN_VOLTAGE);
	ts_printf("> VDAC max out = %7.3f volt\n", DAC_MAX_VOLTAGE);

	dacOutputVoltage = DAC_MIN_VOLTAGE;

	while (true)
	{
		// verhoog de uitgangsspanning stapsgewijs van -10 V naar +10 V
		while ((dacOutputVoltage + stepSize) <= DAC_MAX_VOLTAGE)
		{
			dacOutputVoltage += stepSize;
			dac.SetOutputVoltage(dacChannel, dacOutputVoltage);
			vTaskDelay(pdMS_TO_TICKS(1));
		}
		// verlaag de uitgangsspanning stapsgewijs van +10 V naar -10 V
		while ((dacOutputVoltage - stepSize) >= DAC_MIN_VOLTAGE)
		{
			dacOutputVoltage -= stepSize;
			dac.SetOutputVoltage(dacChannel, dacOutputVoltage);
			vTaskDelay(pdMS_TO_TICKS(1));
		}
	}
	
	// hier komen we nooit!
	vTaskDelete(NULL);
}

///////////////////////////////////////////////////////////////////////////////
// setup
//
// Arduino roept setup() één keer aan tijdens het opstarten. De volgorde is
// belangrijk: eerst de seriële uitvoer, daarna de gedeelde uitvoertaak
// (ts_printf), de heartbeat, de systeeminformatie-commando's, de
// applicatietaken en tot slot de commandoconsole. De korte vertragingen geven
// elk subsysteem tijd om op te starten voordat het volgende het gebruikt.

void setup (void)
{
	Serial.begin(115200);
	delay(1000);
	Serial.println("System initializing");
	// Start de taak die ts_printf en ts_debug gebruiken voor geserialiseerde uitvoer.
	StartTsPrintfTask(NULL);

	// Laat de LED op het ESP32-board knipperen als heartbeat, zodat los van de
	// applicatietaken zichtbaar is dat het systeem draait.
	StartHeartbeatTask(LED_PIN_ESP32_BOARD);
	delay(500);
	// Voeg commando's toe die systeeminformatie via de console beschikbaar maken.
	RegisterSystemInfoCommands();
	delay(500);

	// Initialiseer de SPI-bus voordat de SPI-randapparatuur wordt gebruikt.
	spi.Init();
	// Initialiseer de DAC-hardware voordat de analoge uitgangen worden aangestuurd.
	dac.Init(&spi);

	// Start de applicatietaken.
	StartUserTasks();
	
	// Start de interactieve commandoconsole nu de gedeelde services bestaan.
	StartCommandConsoleTask(NULL);

	delay(500);
}

void loop(void)
{
	// Het eigenlijke werk gebeurt in FreeRTOS-taken. Arduino roept loop() nog
	// steeds aan; de delay geeft de CPU vrij in plaats van actief te wachten.
  delay(500);
}
