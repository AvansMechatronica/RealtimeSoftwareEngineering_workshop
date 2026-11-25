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

xTaskHandle handle_UserTask		= NULL;
xTaskHandle handle_UserTask_2	= NULL;

///////////////////////////////////////////////////////////////////////////////
// Functieprototypes
//
// Deze declaraties maken de taak- en opstartfuncties bekend vóór hun
// implementatie verderop. Een FreeRTOS-taak krijgt altijd een void-pointer
// als parameter, ook als de taak zelf geen parameters nodig heeft.

void UserTask(void *pvParameters);
void UserTask_2(void *pvParameters);

void StartUserTasks(void);

dio_device dio;

void dirtyDelay(void);

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
	
	result = xTaskCreate(UserTask, "tsk_User", (configMINIMAL_STACK_SIZE), NULL, priority, &handle_UserTask);
	if (result == pdPASS )
	{
		// De taak is succesvol aangemaakt. De scheduler roept UserTask aan
		// zodra de taak processortijd krijgt.
	}
	// Als het aanmaken mislukt, blijft de task-handle NULL. Er is hier geen
	// herstelactie, dus de applicatie draait verder zonder UserTask.

	result = pdFAIL;
	priority = 1;

	result = xTaskCreate(UserTask_2, "tsk_User_2", (configMINIMAL_STACK_SIZE), NULL, priority, &handle_UserTask_2);
	if (result == pdPASS )
	{
		// De taak is succesvol aangemaakt. De scheduler roept UserTask_2 aan
		// zodra de taak processortijd krijgt.
	}
	// Als het aanmaken mislukt, blijft de task-handle NULL. Er is hier geen
	// herstelactie, dus de applicatie draait verder zonder UserTask_2.


}





///////////////////////////////////////////////////////////////////////////////
// UserTask
//
// Zet herhaaldelijk één uitgangsbit aan en weer uit. De aangesloten LED
// knippert daardoor, een eenvoudige visuele indicatie dat de taak leeft en
// door de scheduler wordt ingepland.
//
// pvParameters hoort bij de standaard FreeRTOS-taaksignatuur. Deze taak heeft
// geen parameters nodig, dus de parameter wordt bewust niet gebruikt.

void UserTask(void *pvParameters)
{

	// Meld dat de taak gestart is. Deze melding gebruikt ts_printf() en is dus
	// altijd zichtbaar, onafhankelijk van de DEBUG-buildoptie.
	ts_printf("> UserTask started\n");

	uint8_t ledNr = 0;
	
	while (true)
	{
		// Let op: ts_printf() print altijd. Gebruik ts_debug() voor meldingen die
		// alleen in een build met DEBUG gedefinieerd zichtbaar moeten zijn.
		ts_printf("> thread 1\n");
		// Zet de gekozen LED aan, wacht, en zet hem daarna weer uit.
		dio.SetBit(ledNr);
		vTaskDelay(pdMS_TO_TICKS(1000));
		dio.ClearBit(ledNr);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
	
	// De lus is bewust oneindig, dus deze regel wordt normaal nooit bereikt.
	// Hij blijft staan als correcte opruimactie voor het geval de lus later
	// wel kan eindigen.
	vTaskDelete(NULL);
}

void UserTask_2(void *pvParameters)
{

	// Meld dat de taak gestart is. Deze melding gebruikt ts_printf() en is dus
	// altijd zichtbaar, onafhankelijk van de DEBUG-buildoptie.
	ts_printf("> UserTask_2 started\n");

	uint8_t ledNr = 1;
	
	while (true)
	{
		// Let op: ts_printf() print altijd. Gebruik ts_debug() voor meldingen die
		// alleen in een build met DEBUG gedefinieerd zichtbaar moeten zijn.
		ts_printf("> thread 2\n");
		// Zet de gekozen LED aan, wacht, en zet hem daarna weer uit.
		dio.SetBit(ledNr);
		vTaskDelay(pdMS_TO_TICKS(500));
		dio.ClearBit(ledNr);
		vTaskDelay(pdMS_TO_TICKS(500));
	}
	
	// De lus is bewust oneindig, dus deze regel wordt normaal nooit bereikt.
	// Hij blijft staan als correcte opruimactie voor het geval de lus later
	// wel kan eindigen.
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

	dio.Init();

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
