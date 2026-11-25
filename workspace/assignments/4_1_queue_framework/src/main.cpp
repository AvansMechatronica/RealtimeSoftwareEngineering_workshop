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

xTaskHandle handle_TaskProducer		= NULL;
xTaskHandle handle_TaskConsumer		= NULL;
xTaskHandle handle_TaskMonitor		= NULL;

///////////////////////////////////////////////////////////////////////////////
// Functieprototypes
void StartUserTasks(void);
void TaskProducer(void *pvParameters);
void TaskConsumer(void *pvParameters);
void TaskMonitor(void *pvParameters);

///////////////////////////////////////////////////////////////////////////////
// HAL-includes voor het RTSW-board
#include "spi_lib.h"
#include "adc_3208_lib.h"

///////////////////////////////////////////////////////////////////////////////
// SPI- en ADC-instanties
//
// Deze objecten representeren de SPI-interface en de ADC-hardware (MCP3208).
// Ze worden in de applicatie gebruikt voor analoog-digitaalconversies.
spi_device spi;
adc3208 adc;

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
	uint8_t priority = 0;
	
	uint8_t queueSize = 5;
	// TODO: queue aanmaken met globale queue handle
		
	result = xTaskCreate(TaskProducer, "tsk_Prod", (configMINIMAL_STACK_SIZE * 2), NULL, priority, &handle_TaskProducer);
	if (result == pdPASS )
	{
	}

	priority = 0;
	result = xTaskCreate(TaskConsumer, "tsk_Cons", (configMINIMAL_STACK_SIZE * 2), NULL, priority, &handle_TaskConsumer);
	if (result == pdPASS )
	{
	}

	priority = 0;
	result = xTaskCreate(TaskMonitor, "tsk_Mon", (configMINIMAL_STACK_SIZE * 2), NULL, priority, &handle_TaskMonitor);
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
	
	ts_printf("> Task Producer started\n");

	while (true)
	{
		// TODO: vul de volgende code aan:
		//		ADC uitlezen kanaal 0
		//		bepaal of de queue vol is
		//		zet meetwaarde van ADC in de queue (wacht desnoods
		//      oneindig lang totdat er plaats is in de queue)
		//		wacht 500 ms
	}
	
	// hier komen we nooit!
	vTaskDelete(NULL);
}


///////////////////////////////////////////////////////////////////////////////
// void TaskConsumer(void *pvParameters)

void TaskConsumer(void *pvParameters)
{
	uint32_t requiredNrOfSamples = 5;
	uint8_t itemsInQueue = 0;
	
	ts_printf("> Task Consumer started\n");

	while (true)
	{
		// TODO: vul de volgende code aan:
		// wacht totdat queue voldoende samples bevat
		// lees het aantal benodigde samples uit de queue
		// bereken het gemiddelde
		// laat het gemiddelde zien
	}
	
	// hier komen we nooit!
	vTaskDelete(NULL);
}


///////////////////////////////////////////////////////////////////////////////
// void TaskMonitor(void *pvParameters)

void TaskMonitor(void *pvParameters)
{
	uint8_t itemsInQueue = 0;
	
	ts_printf("> Task Monitor started\n");

	while (true)
	{
		// TODO:
		// laat het aantal items in de queue zien
		// wacht 200 ms
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
	// Initialiseer de ADC-hardware voordat de analoge ingangen worden gelezen.
	adc.Init(&spi);

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
