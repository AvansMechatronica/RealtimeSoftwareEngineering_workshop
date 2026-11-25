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
#include "spi_lib.h"
#include "adc_3208_lib.h"

///////////////////////////////////////////////////////////////////////////////
// FreeRTOS task-handles
//
// Een handle wordt door xTaskCreate() ingevuld zodra de taak is aangemaakt.
// Daarmee kan de taak later worden geïdentificeerd, gepauzeerd (suspend),
// hervat (resume) of verwijderd (delete). De beginwaarde NULL geeft aan dat
// de taak nog niet bestaat zolang setup() hem niet heeft gestart.

xTaskHandle handle_TaskADC[2]		= {NULL, NULL};

///////////////////////////////////////////////////////////////////////////////
// SPI- en ADC-instanties
//
// Deze objecten representeren de SPI-interface en de ADC-hardware (MCP3208).
// Ze worden in de applicatie gebruikt voor analoog-digitaalconversies.
spi_device spi;
adc3208 adc;


///////////////////////////////////////////////////////////////////////////////
// Functieprototypes
//
// Deze declaraties maken de taak- en opstartfuncties bekend vóór hun
// implementatie verderop. Een FreeRTOS-taak krijgt altijd een void-pointer
// als parameter, ook als de taak zelf geen parameters nodig heeft.

void TaskADC(void *pvParameters);

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
	uint32_t adcChannel = 4; // kanaal van potmeter RV1/P0

	result = xTaskCreate(TaskADC, "tsk_ADC1", (configMINIMAL_STACK_SIZE * 2), (void*)(adcChannel), priority, &handle_TaskADC[0]);
	if (result == pdPASS )
	{
		// De taak is succesvol aangemaakt. De scheduler roept TaskADC aan
		// zodra de taak processortijd krijgt.
	}
	// tweede taak voor ADC-kanaal 5
	adcChannel = 5; // kanaal van potmeter RV2/P1
	result = xTaskCreate(TaskADC, "tsk_ADC2", (configMINIMAL_STACK_SIZE * 2), (void*)(adcChannel), priority, &handle_TaskADC[1]);
	if (result == pdPASS )
	{
		// De taak is succesvol aangemaakt. De scheduler roept TaskADC aan
		// zodra de taak processortijd krijgt.
	}
}


///////////////////////////////////////////////////////////////////////////////
// TaskADC
//
// Leest ADC-waarden van het opgegeven kanaal en verwerkt ze. De taak meet
// analoge ingangssignalen en levert de gegevens voor verdere verwerking of weergave.
//
// pvParameters hoort bij de standaard FreeRTOS-taaksignatuur. Deze taak krijgt
// daarmee het nummer van het te meten ADC-kanaal mee.

void TaskADC(void *pvParameters)
{
	double adcValue = 0;
	uint32_t channel = (uint32_t)(pvParameters);
	
	ts_printf("> Task ADC started, channel %lu\n", channel);

	// meet en toon het opgegeven ADC-kanaal (channel), elke seconde
	while (1)
	{
		adcValue = adc.ReadVoltage(channel);
		ts_printf("ADC channel %lu: %.2f V\n", channel, adcValue);
		vTaskDelay(pdMS_TO_TICKS(1000));
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
