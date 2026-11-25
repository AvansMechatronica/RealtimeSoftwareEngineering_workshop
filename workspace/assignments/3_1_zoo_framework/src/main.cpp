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
// Geen

///////////////////////////////////////////////////////////////////////////////
// FreeRTOS task-handles
//
// Een handle wordt door xTaskCreate() ingevuld zodra de taak is aangemaakt.
// Daarmee kan de taak later worden geïdentificeerd, gepauzeerd (suspend),
// hervat (resume) of verwijderd (delete). De beginwaarde NULL geeft aan dat
// de taak nog niet bestaat zolang setup() hem niet heeft gestart.

xTaskHandle handle_UserTask		= NULL;

///////////////////////////////////////////////////////////////////////////////
// Functieprototypes
//
// Deze declaraties maken de taak- en opstartfuncties bekend vóór hun
// implementatie verderop. Een FreeRTOS-taak krijgt altijd een void-pointer
// als parameter, ook als de taak zelf geen parameters nodig heeft.

void UserTask(void *pvParameters);
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
	
	result = xTaskCreate(UserTask, "tsk_User", (configMINIMAL_STACK_SIZE * 2), NULL, priority, &handle_UserTask);
	if (result == pdPASS )
	{
		// De taak is succesvol aangemaakt. De scheduler roept UserTask aan
		// zodra de taak processortijd krijgt.
	}
	// Als het aanmaken mislukt, blijft de task-handle NULL. Er is hier geen
	// herstelactie, dus de applicatie draait verder zonder UserTask.

	// TODO: maak vier taken aan, één per entreepoortje, en geef het nummer van
	//       het poortje (1..4) mee via pvParameters.
	// TODO: bescherm daarna de gedeelde teller G_NumberOfVisitors met een mutex
	//       (xSemaphoreCreateMutex) en vergelijk het eindresultaat.
}

uint32_t G_NumberOfVisitors = 0;
///////////////////////////////////////////////////////////////////////////////
// UserTask
//
// Telt de bezoekers bij één entreepoortje van de dierentuin. Elke instantie
// van deze taak hoort bij één poortje en verhoogt dezelfde globale teller
// G_NumberOfVisitors. Daarna gaat de taak voor altijd slapen.
//
// pvParameters bevat het nummer van het entreepoortje (zooEntry).

void UserTask(void *pvParameters)
{
	uint32_t zooEntry = (uint32_t) (pvParameters);
	
	ts_printf("> ZooEntry %d started\n", zooEntry);

	// TODO: tel gespecificeerd aantal bezoekers voor het entreepoortje
	
	ts_printf("> ZooEntry %lu: Total visitors = %lu\n", zooEntry, G_NumberOfVisitors);
	
	// TODO: na het tellen voor altijd gaan slapen (oneindige lus met vTaskDelay)
	
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
