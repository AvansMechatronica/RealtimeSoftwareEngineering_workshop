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

xTaskHandle handle_TaskTimerInterruptHandler		= NULL;

///////////////////////////////////////////////////////////////////////////////
// Functieprototypes
//
// Deze declaraties maken de taak- en opstartfuncties bekend vóór hun
// implementatie verderop. Een FreeRTOS-taak krijgt altijd een void-pointer
// als parameter, ook als de taak zelf geen parameters nodig heeft.

void TaskTimerInterruptHandler(void *pvParameters);
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

///////////////////////////////////////////////////////////////////////////////
// void StartUserTasks(void)

void StartUserTasks(void)
{
	BaseType_t result = pdFAIL;
	uint8_t priority = 0;
	
	result = xTaskCreate(TaskTimerInterruptHandler, "tsk_TIH", (configMINIMAL_STACK_SIZE), NULL, priority, &handle_TaskTimerInterruptHandler);
	if (result == pdPASS )
	{
	}

}


#define LED_PIN 15 // GPIO15 is direct verbonden met de LED op het RTSW-board
#define interrupt_pin 36 // GPIO36 is verbonden met de interruptingang van het RTSW-board (D0), pin 1 van DB9-connector J11
// Verbind deze pin met één van de klokuitgangen van de klokgenerator op DB9-connector J6

///////////////////////////////////////////////////////////////////////////////
// void ClockInterruptHandler(void)
//
// Dit is de Interrupt Service Routine (ISR) - maak deze zo kort mogelijk!
// Deze wordt elke milliseconde uitgevoerd (f = 1 KHz, T = 1 ms)
//
// ***********************************************************************
// Deze functie wordt NIET aangeroepen vanuit het programma, maar wordt
// door de CPU "aangeroepen" op het moment dat er een clock interrupt optreedt.
// Deze functie wordt gekoppeld aan de clock interruptbron m.b.v. de functie
// attachInterrupt()
//
// ***********************************************************************

// TODO: declareer variabele g_InterruptCount met beginwaarde 0
// TODO: declareer semafoorhandle handle_InterruptSemaphore met beginwaarde NULL

// De ESP32 Arduino-core roept een GPIO-interrupthandler aan zonder argumenten.
// IRAM_ATTR zorgt dat de ISR in het interne RAM staat, zodat hij altijd snel
// uitgevoerd kan worden.
void IRAM_ATTR ClockInterruptHandler(void)
{
	BaseType_t higherPriorityTaskWoken = pdFALSE;

	// TODO: volgende deel afmaken:
	
	/*
	if (<je eigen semafoor handle> != NULL)
	{
		// TODO: verhoog interrupt count
		// TODO: ALS (interrupt count % 1000) == 0: (dus elke seconde)
		// TODO: DAN deblokkeer TaskTimerInterruptHandler met semafoor,
		//       gebruik xSemaphoreGiveFromISR(..., &higherPriorityTaskWoken)
	}
	*/

	portYIELD_FROM_ISR(higherPriorityTaskWoken);
}

///////////////////////////////////////////////////////////////////////////////
// void TaskTimerInterruptHandler(void *pvParameters)
//
// Deze taak is de deferred interrupt handler: hij wordt elke seconde door de
// ISR gedeblokkeerd.

void TaskTimerInterruptHandler(void *pvParameters)
{
	uint32_t taskInterruptCount = 0;

	ts_printf("> Task Timer InterruptHandler started\n");
	
	// TODO: maak counting semafoor met juiste beginwaarden. Doe dit vóór het
	//       koppelen van de interrupt, zodat de ISR de semafoor direct kan gebruiken.
	// uint32_t maxSemCount	= ?;
	// uint32_t initialSemCount = ?;
	
	// TODO: koppel de interrupthandler aan de klok:
	//		juiste pin: GPIO36 (D0, pin 1 van DB9-connector J11), verbonden met
	//		een klokuitgang (1 kHz) van de klokgenerator op DB9-connector J6
	//		juiste flank
	// Gebruik pinMode() en attachInterrupt(); attachInterrupt() zet de interrupt
	// ook meteen aan.
	
	while (true)
	{
		// TODO: wacht op semafoor, deze wordt ELKE SECONDE gedeblokkeerd in de ISR
		//		 verhoog taskInterruptCount met één 
		
		ts_printf("> Interrupt!\n");
		
		// TODO:	laat waarde zien van globale variabele die de "harde" klokinterrupts telt
		//			(== g_InterruptCount, elke milliseconde) én de variabele die de taak interrupts 
		//			telt (== taskInterruptCount, elke seconde): deze moet dus een factor 
		//			1000 kleiner zijn!
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
