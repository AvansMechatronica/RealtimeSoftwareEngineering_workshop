# Opdrachten 2: Tasks en priorities in FreeRTOS

In dit practicum komen de volgende onderwerpen aan bod:

- Het maken van tasks (= taak, threads) in FreeRTOS
- Het wijzigen van task priorities
- Het gebruik van de functie ts_printf
- Het gebruik van delays en het effect daarvan op de uitvoering van tasks


## Aansturen van de LED’s op het NodeMCU-Shield

In dit practicum worden enkele van de 6 LED’s op het NodeMCU-Shield aangestuurd. De LED's zijn aangesloten op de digitale uitgangen D1 t/m D6. Voor het aansturen van een LED  wordt de volgende functie gebruikt:

```cpp
// Zet de LED aan op basis van het ledNumber
dio.SetBit(ledNumber);
// Zet de LED uit op basis van het ledNumber
dio.ClearBit(ledNumber);
```

ledNumber is het nummer van de aan te sturen LED, en moet liggen ligt tussen 0 en 5:
- 0 = meest RECHTSE LED, bit 0, Least significant Bit (aangegeven op het board met D1)
- 5 = meest LINKSE LED, bit 5, Most Significant bit (aangegeven op het board met D6)

De `setBit` en de `clearBit` member-functies van het `dio` object van de `dio_device` klasse.
Om deze klasse te gebruiken, moet eerst een object van het type `dio_device` worden aangemaakt, zoals hieronder weergegeven.

```cpp
// HAL includes for RTSW board
#include "dio_lib.h"

dio_device dio;
```

## 2.1 Tasks in FreeRTOS
::::{admonition} Opdracht 2.1.1
:class: tip

  * Sluit het `NodeMCU32`board via een USB kabel aan op de computer. Sluit de `+12V in` aan met een geschikte power-adapter.

  * Open in visual code de folder/map `<path-to-your-project>/assignments/1_assignment/1_basics_framework`.

:::{note}
Zie [installatie-instructies](install.md#verkrijgen-van-de-workshopbestanden) voor instructies over het installeren van de benodigde software.
:::

  * Compileer en download het programma naar de NodeMCU32 met de `Upload` knop in de menubalk onder in het scherm

![image](../images/Upload.JPG)

  * Let goed op de mededelingen in de terminal van `Visual Code`, hieraan kun je zien of het uploaden succesvol is verlopen.

  * Als het programma correct is geüpload, zou een LED op de NodeMCU-Shield moeten gaan knipperen.

  * Tevens zal er een led knipperen op het ESP32 board zelf. Dit is een z.g.n. `hartbeat` LED, welke aangeeft dat het systeem actief is. Deze hartbeat is in alle software van deze course aanwezig. Als de led niet knippert, is de ESP32 mogelijk `vastgelopen` en moet mogelijk opnieuw worden opgestart. Je kunt dit doen met de `RESET` knop op het board.

  * Bestudeer de code in `src/main.c` om te begrijpen hoe de LED wordt aangestuurd en hoe de task is opgezet. Deze code moet in de volgende opdracht worden aangepast c.q. worden uitgebreid.
::::

:::{warning}
Voeg uitsluitend code toe aan, of wijzig code in, het bestand `src/main.c`. Alle overige folders bevatten bestanden voor de systeemconfiguratie, bibliotheken, enz. Laat deze **ongewijzigd**.
:::

In de functie `StartUserTasks()` is onder andere een task gecreëerd die een LED laat knipperen. De code die door deze task wordt uitgevoerd, staat in de functie `UserTask`. Deze task wordt gemaakt en gestart met de volgende code, die gebruikmaakt van de FreeRTOS-functie `xTaskCreate`:

```cpp
result = xTaskCreate(UserTask, 
                     "tsk_User",
					 configMINIMAL_STACK_SIZE,
					 NULL, 
					 priority, 
					 &handle_UserTask);
```

De parameters voor deze functie zijn achtereenvolgens:

- `UserTask`: de functie die door de task wordt uitgevoerd. Bekijk in de broncode wat deze functie doet.
- `"tsk_User"`: de human-readable naam waaronder deze task wordt uitgevoerd. Deze naam wordt onder andere gebruikt door de commando's `tasks` en `run-time-stats`.
- `configMINIMAL_STACK_SIZE`: de grootte van de stack (de toegewezen geheugenruimte) in bytes.
- `NULL`: de parameter die aan de task kan worden meegegeven. Voorlopig is deze `NULL` (een null-pointer, oftewel ongebruikt).
- `priority`: de prioriteit van de task. Tasks met een hogere prioriteit hebben bij het gebruik van de CPU voorrang op tasks met een lagere prioriteit. Prioriteit 0 is het laagst en prioriteit 4 het hoogst in deze practicumconfiguratie.
- `&handle_UserTask`: het adres van de handle die `xTaskCreate` aan de aanroeper teruggeeft. Met deze handle kan later, indien nodig, naar deze task worden verwezen.

:::{admonition} Opdracht 2.1.2
:class: tip
Open de Terminal. Je kunt deze openen met het volgende icoon aan te klikken:

![image](../images/serial_monitor_icon.png)

**Voorbeelden van uitvoer**

Voorbeeld van de programma-uitvoer in Console/Terminal:

*Figuur 1. Console/Terminal terminal window*

```
System initializing
> Heartbeat should be running, flashing onboard LED, pin 2...
FreeRTOS command server
Available commands:
  version    Show firmware information
  cpuinfo    Show ESP32 CPU/chip information
  memory     Show ESP32 heap memory information
  tasks      Show current task snapshot
  help       Show this help
  echo       Echo text, for example: echo hello
> UserTask started
> thread 1
> thread 1
> thread 1
```

Voer in deze Console/Terminal het commando `tasks` uit om de status van alle actieve tasks te bekijken. Je zou nu `tsk_User` in de lijst moeten  zien, met prioriteit 0.

Voorbeeld van `tasks` commando:
```
--- 11 tasks executing ---
Name                 State  Prio  Stack
----------------------------------------
CommandConsole       RUN       1   1328
IDLE0                READY     0    576
tsk_User             READY     0     88
IDLE1                READY     0    568
loopTask             BLOCK     1   7244
tsk_Heartbeat        BLOCK     1    860
esp_timer            SUSP     22   8160
Tmr Svc              BLOCK     1   3596
ipc1                 SUSP     24    480
tsk_Printf           BLOCK    23    700
ipc0                 SUSP     24    484
```

:::


:::{admonition} Opdracht 2.1.3
:class: tip

Voeg code toe voor een nieuwe (identieke) task die LED D2 laat knipperen. LED D1 blijft dus óók (ongewijzigd) knipperen

**Aanwijzingen**

- Declareer een variabele van het juiste type met de naam `handle_UserTask_2`.
- Maak een functie `UserTask_2` die LED D2 laat knipperen. Deze functie wordt dus bijna een kopie van `UserTask`.
- Voeg aan de functie `StartUserTasks()` de bijbehorende code toe om `UserTask_2` te starten.
- Geef de nieuwe task de naam `"tsk_User_2"`.
- Geef `UserTask_2` dezelfde prioriteit als `UserTask`; beide hebben prioriteit 0.

:::

:::{admonition} Opdracht 2.1.4
:class: tip
Voer in deze Terminal het commando `tasks` uit om de status van alle actieve tasks te bekijken. Je zou nu zowel `tsk_User` als `tsk_User_2` in de lijst moeten zien, beide met prioriteit 0.

:::
## 2.2 Task priorities in FreeRTOS

Het resultaat van de vorige opdrachten is een programma met twee onafhankelijke tasks, waarbij elke task één LED laat knipperen. Beide tasks hebben dezelfde prioriteit, namelijk 0.

:::{admonition} Opdracht 2.2.1
:class: tip
 verhoog de prioriteit van de laatst toegevoegde task UserTask_2 die LED D2 laat knipperen (met de naam “tsk_User_2”) van 0 naar 1. Doe dit door de prioriteit de waarde 1 te geven bij xTaskCreate. Bepaal het effect van deze prioriteitswijziging op het knipperen van beide LED’s.

**Aanwijzingen**

- Gebruik de task handle die bij `xTaskCreate` is aangemaakt.
- De prioriteit van `UserTask_2` kan op elk willekeurig moment door iedereen worden aangepast, dus ook bijvoorbeeld in `UserTask`.

:::

:::{admonition} Opdracht 2.2.2
:class: tip
Gebruik het commando `tasks` om het effect te zien op de programma-uitvoering en het CPU-gebruik.
:::

:::{admonition} Opdracht 2.2.3
:class: tip
bepaal welke functie(s) er is / zijn in FreeRTOS om de prioriteit van een task te wijzigen, NADAT deze is gestart (= run-time wijzigen), EN bepaal wat hierbij de functieparameters zijn.
:::

:::{admonition} Opdracht 2.2.4
:class: tip
Gebruik deze functie om run-time de prioriteit te wijzigen van task UserTask_2 (opgestart met prioriteit 1) naar prioriteit 2, en controleer dit m.b.v. het tasks commando.
**Aanwijzingen**
* Gebruik de task handle die bij xTaskCreate is aangemaakt
* De prioriteit van UserTask_2 kan op elk willekeurig moment door iedereen worden aangepast, dus ook bv. in User_Task!
 
:::


## 2.3 Delay- en sleepfuncties

Een task hoeft niet altijd bezig te zijn met de uitvoering van een programma. Tijdens de uitvoering bevindt een task zich in de `ready`- of `running`-state. Een task kan ook wachten op een bepaalde gebeurtenis of totdat een bepaalde tijd is verstreken. In dat geval bevindt de task zich in de zogenoemde `blocked`-state.

De tasks uit de voorgaande opdrachten bevonden zich altijd in de `running`- of `ready`-state. Dat is ook het geval wanneer een task met de functie `dirtyDelay` actief staat te wachten totdat een bepaalde tijd is verstreken.

De functie `dirtyDelay` is echter zeer inefficiënt: de CPU voert hierin zinloze acties uit, zoals optellen, om tijd te doden. Het is veel efficiënter om een task in de `blocked`-state te zetten en de task te laten wachten totdat de wachttijd voorbij is. Daarna kan de task weer tijdelijk worden uitgevoerd. Gedurende die wachttijd kunnen andere tasks door de CPU worden uitgevoerd. Dit kan worden gerealiseerd door `dirtyDelay` te vervangen door een efficiëntere FreeRTOS-delayfunctie.

:::{admonition} Opdracht 2.3.1
:class: tip
Bepaal de functionaliteit van de FreeRTOS vTaskDelay functie én wat de eenheid is van de bijbehorende parameter die de delaytijd bepaalt (lees de documentatie in de naslagwerken, of Google deze).
:::

:::{admonition} Opdracht 2.3.2
:class: tip
Bepaal welke FreeRTOS functies er nog meer zijn die gebruikt kunnen worden voor het maken van delays.
:::

De functie `vTaskDelay` werkt met clock ticks van het systeem. Het is echter vaak makkelijker om tijden in (milli)seconden te specificeren. Hiervoor is de bibliotheekfunctie `taskSleep` beschikbaar. Deze functie maakt gebruik van `vTaskDelay` en rekent de gewenste tijd, gespecificeerd in milliseconden, om naar het bijbehorende aantal ticks.

:::{admonition} Opdracht 2.3.3
:class: tip
Bekijk de implementatie van de functie taskSleep in de file TaskSleep.c, en kijk hoe de deze omrekening gebeurt.
:::

Bij de volgende opdrachten wordt gebruikgemaakt van de code zoals die tot en met opdracht 2.2.4 is gemaakt. Dat betekent dat `UserTask` prioriteit 0 heeft en `UserTask_2` prioriteit 1 of 2, in elk geval hoger dan 0. Het effect hiervan is dat de task met de hoogste prioriteit (1 of 2) alle CPU-tijd krijgt, waardoor alleen de bijbehorende LED knippert. De andere LED blijft uit, omdat de bijbehorende task een lagere prioriteit heeft (0) en dus niet aan de beurt komt.

:::{admonition} Opdracht 2.3.4
:class: tip
Pas de code in User_Task en User_task_2 zodanig aan, dat LED 1 knippert met een frequentie van ca. 1 Hz (dus 1x / sec) en LED 2 met een frequentie van 2 Hz (dus 2x / sec). Maak uiteraard gebruik van de functie taskSleep!
:::

:::{admonition} Opdracht 2.3.5
:class: tip
Bepaal wat het effect is van het gebruik van taskSleep op het knipperen van beide LED’s, en beredeneer waarom beide LED’s nu wél knipperen (wat niet het geval was bij gebruik van dirtyDelay, daar knipperde slechts één LED).
:::


