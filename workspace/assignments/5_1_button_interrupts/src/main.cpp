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
// HAL-includes voor het RTSW-board
#include "dio_lib.h"

///////////////////////////////////////////////////////////////////////////////
// FreeRTOS task-handles
//
// Een handle wordt door xTaskCreate() ingevuld zodra de taak is aangemaakt.
// Daarmee kan de taak later worden geïdentificeerd, gepauzeerd (suspend),
// hervat (resume) of verwijderd (delete). De beginwaarde NULL geeft aan dat
// de taak nog niet bestaat zolang setup() hem niet heeft gestart.

xTaskHandle handle_TaskInterruptHandler	= NULL;

///////////////////////////////////////////////////////////////////////////////
// Functieprototypes
//
// Deze declaraties maken de taak- en opstartfuncties bekend vóór hun
// implementatie verderop. Een FreeRTOS-taak krijgt altijd een void-pointer
// als parameter, ook als de taak zelf geen parameters nodig heeft.

void TaskInterruptHandler(void *pvParameters);
void StartUserTasks(void);


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
	
	result = xTaskCreate(TaskInterruptHandler, "tsk_IH", (configMINIMAL_STACK_SIZE), NULL, priority, &handle_TaskInterruptHandler);
	if (result == pdPASS )
	{
		// De taak is succesvol aangemaakt. De scheduler roept TaskInterruptHandler aan
		// zodra de taak processortijd krijgt.
	}
	// Als het aanmaken mislukt, blijft de task-handle NULL. Er is hier geen
	// herstelactie, dus de applicatie draait verder zonder TaskInterruptHandler.
}

// TODO: declareer variabele g_InterruptCount met beginwaarde 0

// TODO: declareer semafoorhandle handle_InterruptSemaphore met beginwaarde NULL

///////////////////////////////////////////////////////////////////////////////
// void interruptHandler(void)
//
// Dit is de Interrupt Service Routine (ISR) - maak deze zo kort mogelijk!
//
// ***********************************************************************
// Deze functie wordt NIET aangeroepen vanuit het programma, maar wordt
// door de CPU "aangeroepen" op het moment dat er een interrupt optreedt.
// Deze functie wordt gekoppeld aan een interruptbron m.b.v. de functie
// attachInterrupt()
// ***********************************************************************

#define BUTTON_0	4 // alleen button 0 wordt gebruikt en is rechtstreeks op de esp32 aangesloten, de andere buttons zijn via een I2C expander aangesloten en worden niet gebruikt

// De ESP32 Arduino-core roept een GPIO-interrupthandler aan zonder argumenten.
// IRAM_ATTR zorgt dat de ISR in het interne RAM staat, zodat hij altijd snel
// uitgevoerd kan worden.
void IRAM_ATTR interruptHandler(void)
{
	BaseType_t higherPriorityTaskWoken = pdFALSE;

	// TODO: volgende deel afmaken:
	
	/*
	if (<je eigen semafoor handle> != NULL)
	{
		// TODO: verhoog interrupt count
		// TODO: deblokkeer TaskInterruptHandler met semafoor,
		//       gebruik xSemaphoreGiveFromISR(..., &higherPriorityTaskWoken)
	}
	*/

	portYIELD_FROM_ISR(higherPriorityTaskWoken);
}

///////////////////////////////////////////////////////////////////////////////
// void TaskInterruptHandler(void *pvParameters)
//
// Deze task is de deferred interrupt handler

void TaskInterruptHandler(void *pvParameters)
{
	ts_printf("> Task InterruptHandler started\n");
	
	// TODO: maak counting semafoor met juiste beginwaarden. Doe dit vóór het
	//       koppelen van de interrupt, zodat de ISR de semafoor direct kan gebruiken.
	
	// TODO: koppel de interrupthandler aan drukknop BUTTON_0 (GPIO4):
	//		juiste pin en pinMode (INPUT_PULLUP)
	//		juiste flank (de knop trekt de pin naar massa)
	// Gebruik pinMode() en attachInterrupt(); attachInterrupt() zet de interrupt
	// ook meteen aan. De andere knoppen zitten via een I2C-expander en worden
	// hier niet gebruikt.
	
	while (true)
	{
		// TODO: wacht op semafoor, deze wordt gedeblokkeerd in de ISR
		
		ts_printf("> Interrupt!\n");
		
		// TODO: laat waarde zien van globale variabele die de interrupts telt
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
