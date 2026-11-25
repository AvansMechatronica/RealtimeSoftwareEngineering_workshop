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

xTaskHandle handle_TaskADC		= NULL;
xTaskHandle handle_TaskDisplay	= NULL;

SemaphoreHandle_t semaADC	  = NULL;
SemaphoreHandle_t semaDisplay = NULL;

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
void TaskDisplay(void *pvParameters);

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

	// maak 2 counting semaforen semaADC en semaDisplay met de juiste (!!) beginwaarden:
	// semaADC begint vrij (1), semaDisplay begint bezet (0)
	uint8_t semaMaxCount		 = 1;
	uint8_t semaInitialCountFree = 0;

	semaADC		= xSemaphoreCreateCounting(semaMaxCount, 1);
	semaDisplay = xSemaphoreCreateCounting(semaMaxCount, semaInitialCountFree);


	result = xTaskCreate(TaskDisplay, "tsk_Display", (configMINIMAL_STACK_SIZE * 2), NULL, priority, &handle_TaskDisplay);
	if (result == pdPASS )
	{
		// De taak is succesvol aangemaakt. De scheduler roept TaskDisplay aan
		// zodra de taak processortijd krijgt.
	}
	result = xTaskCreate(TaskADC, "tsk_ADC", (configMINIMAL_STACK_SIZE * 2), NULL, priority, &handle_TaskADC);
	if (result == pdPASS )
	{
		// De taak is succesvol aangemaakt. De scheduler roept TaskADC aan
		// zodra de taak processortijd krijgt.
	}
}


///////////////////////////////////////////////////////////////////////////////
// TaskADC
//
// Leest alle ADC-kanalen in de globale array G_ADCValue. Met de semaforen
// semaADC en semaDisplay wordt de taak gesynchroniseerd met TaskDisplay:
// TaskADC schrijft pas nieuwe waarden als TaskDisplay de vorige heeft getoond.
//
// pvParameters wordt in deze opdracht niet gebruikt.

double G_ADCValue[N_ADC_CHANNELS] = {0};

void TaskADC(void *pvParameters)
{
	uint32_t channel = (uint32_t)(pvParameters);
	
	ts_printf("> Task ADC started, channel %lu\n", channel);

	while (true)
	{
		// wacht op juiste semafoor
		if (xSemaphoreTake(semaADC, portMAX_DELAY) == pdTRUE)
		{
			for(int i = 0; i < N_ADC_CHANNELS; i++)
				G_ADCValue[i] = adc.ReadVoltage(i);
			// deblokkeer juiste semafoor
			xSemaphoreGive(semaDisplay);
		}
		vTaskDelay(pdMS_TO_TICKS(2000)); // wacht 2 seconden
	}
	
	// hier komen we nooit!
	vTaskDelete(NULL);
}

///////////////////////////////////////////////////////////////////////////////
// void TaskDisplay(void *pvParameters)
//
// Toont de ADC-waarden uit G_ADCValue zodra TaskADC nieuwe waarden heeft
// gemeten, en geeft daarna TaskADC weer vrij voor een nieuwe meting.

void TaskDisplay(void *pvParameters)
{
	ts_printf("> Task Display started\n");
	uint32_t counter = 0;

	while (true)
	{
		// TODO: wacht op juiste semafoor
		if (xSemaphoreTake(semaDisplay, portMAX_DELAY) == pdTRUE)
		{
			ts_printf("[DISP] Updating display with ADC values, counter = %lu\n", counter);
			for(int i = 0; i < N_ADC_CHANNELS; i++)
				ts_printf("[DISP] ADC value[%d] = %4.2f\n", i, G_ADCValue[i]	);
			counter++;
			// deblokkeer juiste semafoor
			xSemaphoreGive(semaADC);
		}
		vTaskDelay(pdMS_TO_TICKS(2000)); // wacht 2 seconden
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
