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

#include "dio_lib.h"

///////////////////////////////////////////////////////////////////////////////
// SPI- en ADC-instanties
//
// Deze objecten representeren de SPI-interface en de ADC-hardware (MCP3208).
// Ze worden in de applicatie gebruikt voor analoog-digitaalconversies.
spi_device spi;
adc3208 adc;

///////////////////////////////////////////////////////////////////////////////
// FreeRTOS task-handles
//
// Een handle wordt door xTaskCreate() ingevuld zodra de taak is aangemaakt.
// Daarmee kan de taak later worden geïdentificeerd, gepauzeerd (suspend),
// hervat (resume) of verwijderd (delete). De beginwaarde NULL geeft aan dat
// de taak nog niet bestaat zolang setup() hem niet heeft gestart.

xTaskHandle handle_TaskTimerInterruptHandler		= NULL;
xTaskHandle handle_TaskParameterSetting			= NULL;

///////////////////////////////////////////////////////////////////////////////
// Queue waarmee TaskParameterSetting de laatste potmeterwaarde doorgeeft aan
// TaskTimerInterruptHandler.

QueueHandle_t handle_ParameterQueue = NULL;

///////////////////////////////////////////////////////////////////////////////
// Functieprototypes
//
// Deze declaraties maken de taak- en opstartfuncties bekend vóór hun
// implementatie verderop. Een FreeRTOS-taak krijgt altijd een void-pointer
// als parameter, ook als de taak zelf geen parameters nodig heeft.

void TaskTimerInterruptHandler(void *pvParameters);
void StartUserTasks(void);
void TaskParameterSetting(void *pvParameters);

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
	
	UBaseType_t parameterQueueSize = 1;
	uint32_t adcChannel = 4; // kanaal van potmeter RV1/P0

	// queue voor het doorgeven van de potmeterwaarde
	handle_ParameterQueue = xQueueCreate(parameterQueueSize, sizeof(double));
	if (handle_ParameterQueue == NULL)
	{
	}

	result = xTaskCreate(TaskTimerInterruptHandler, "tsk_TIH", (configMINIMAL_STACK_SIZE * 2), NULL, priority, &handle_TaskTimerInterruptHandler);
	if (result == pdPASS )
	{
	}

	result = xTaskCreate(TaskParameterSetting, "tsk_PS", (configMINIMAL_STACK_SIZE * 2), (void*)(adcChannel), priority, &handle_TaskParameterSetting);
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
// attachInterrupt
//
// ***********************************************************************

uint32_t g_InterruptCount = 0;
uint32_t g_id  = 0;
uint32_t g_mask = 0;

xSemaphoreHandle handle_InterruptSemaphore = NULL;

// De ESP32 Arduino-core roept een GPIO-interrupthandler aan zonder argumenten.
// Daarom worden de pin en het bijbehorende bitmasker hier zelf ingevuld, in
// plaats van dat ze worden meegegeven (zoals bij de oude SAM3X-InterruptLib
// van de Arduino DUE).
void IRAM_ATTR ClockInterruptHandler(void)
{
	BaseType_t higherPriorityTaskWoken = pdFALSE;

	if (handle_InterruptSemaphore != NULL)
	{
		g_InterruptCount++;
		g_id = interrupt_pin;
		g_mask = digitalPinToBitMask(interrupt_pin);

		if ((g_InterruptCount % 1000) == 0)
		{
			xSemaphoreGiveFromISR(handle_InterruptSemaphore, &higherPriorityTaskWoken);
		}
	}

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
	TickType_t ticksToWait = 0;
	double potmeterValue = 0.0;

	ts_printf("> Task Timer InterruptHandler started\n");

	uint32_t maxSemCount = 1;
	uint32_t initialSemCount = 0;

	// Maak de semafoor aan vóórdat de interrupt wordt gekoppeld, zodat de ISR
	// hem veilig kan vrijgeven zodra er interrupts binnenkomen.
	handle_InterruptSemaphore = xSemaphoreCreateCounting(maxSemCount, initialSemCount);

	// De externe klokgenerator op J6 stuurt interrupt_pin aan met een blokgolf van
	// 1 kHz, dus elke milliseconde komt er een dalende flank (FALLING).
	// attachInterrupt registreert de handler én zet de interrupt aan.
	pinMode(interrupt_pin, INPUT);
	attachInterrupt(digitalPinToInterrupt(interrupt_pin), ClockInterruptHandler, FALLING);

	while (true)
	{
		xSemaphoreTake(handle_InterruptSemaphore, portMAX_DELAY);
		taskInterruptCount++;

		ts_printf("> Task interrupt!\n");
		ts_printf("> interrupt count = %lu, taskInterruptCount = %lu\n", g_InterruptCount, taskInterruptCount);
		
		// laat de parameterwaarde altijd in de queue staan! Gebruik daarom xQueuePeek
		ticksToWait = 0;
		xQueuePeek(handle_ParameterQueue, &potmeterValue, ticksToWait);
		ts_printf("> potmeter value read: %.1f\n", potmeterValue);

	}
	
	// hier komen we nooit!
	vTaskDelete(NULL);
}


///////////////////////////////////////////////////////////////////////////////
// void TaskParameterSetting(void *pvParameters)
//
// Leest de potmeter, en zet de waarde in de queue zodra hij voldoende
// veranderd is.

void TaskParameterSetting(void *pvParameters)
{
	double adcValue = 0;
	uint32_t channel = (uint32_t)(pvParameters);
	
	double minValue = 1.0;
	double maxValue = 5.0;
	
	double currentValue  = -1.0;	// begin met een ongeldige waarde
	double previousValue =  0.0;
	
	double threshold = 0.1;
	
	ts_printf("> starting TaskParameterSetting\n");


	
	while(true)
	{
		adcValue = adc.ReadVoltage(channel);

		// normaliseer de ADC-waarde (0..2,5 V) naar het bereik minValue..maxValue
		currentValue = fmap(adcValue, ADC47_MIN_VOLTAGE, ADC47_MAX_VOLTAGE, minValue, maxValue);

		if ( fabs(currentValue - previousValue) > threshold )
		{
			xQueueOverwrite(handle_ParameterQueue, &currentValue);
			ts_printf("> value set to %.1f\n", currentValue);
			previousValue = currentValue;
		}

		vTaskDelay(pdMS_TO_TICKS(10)); // wacht 10 ms
	}
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
