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

xTaskHandle handle_QuadratureTask	= NULL;


///////////////////////////////////////////////////////////////////////////////
// Functieprototypes
void StartUserTasks(void);
void TaskQuadrature(void *pvParameters);

///////////////////////////////////////////////////////////////////////////////
// HAL-includes voor het RTSW-board
#include "spi_lib.h"
#include "qc_7366_lib.h"

///////////////////////////////////////////////////////////////////////////////
// SPI- en quadratuurteller-instanties
//
// Deze objecten representeren de SPI-interface en de quadratuurteller (LS7366).
// Ze worden gebruikt om de stand van de quadratuur-encoder uit te lezen.
spi_device spi;
qc7366 qc;

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
	
	result = xTaskCreate(TaskQuadrature, "tsk_Quad", (configMINIMAL_STACK_SIZE * 2), NULL, priority, &handle_QuadratureTask);
	if (result == pdPASS )
	{
	}

}

///////////////////////////////////////////////////////////////////////////////
// void TaskQuadrature(void *pvParameters)
//
// Stelt de quadratuurteller in en leest elke seconde de tellerstand uit.

void TaskQuadrature(void *pvParameters)
{
	// volgende regel niet wijzigen!
	uint8_t mode = MODE_QC_1 | MODE_FREERUNNING | INDEX_RESETCNTR | INDEX_ASYNC | FILTERCLOCK_DIV_2;
	uint8_t qcChannel	= 1;
	int32_t qcCount		= 0;
	uint8_t qcStatus	= 0;

	ts_printf("> Task Quadrature started\n");

	// TODO: stel de quadratuurteller in (mode register 1 met 'mode') en zet de teller aan

	while (true)
	{
		// TODO: vul de volgende code aan:
		//		quadratuurteller uitlezen (kanaal qcChannel) en de stand afdrukken
		//		wacht 1000 ms
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
	// Initialiseer de quadratuurteller voordat hij wordt gebruikt.
	qc.Init(&spi);

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
