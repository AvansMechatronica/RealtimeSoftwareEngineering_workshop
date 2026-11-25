# Opdracht 3: Mutual exclusion en task synchronisation in FreeRTOS

In dit practicum komen de volgende onderwerpen aan bod:

- Het gebruik van mutexes in FreeRTOS voor toegang tot gedeelde resources
- Het synchroniseren van tasks
- Het gebruik van de ADC

## 3.1 Shared resources in FreeRTOS

Gegeven de volgende situatie: in een dierentuin zijn er vier entreepoortjes, waarmee het totaal aantal bezoekers in de dierentuin moet worden geteld. Er worden alleen inkomende bezoekers geteld.

- Het totale aantal inkomende bezoekers wordt opgeslagen in de globale variabele `G_NumberOfVisitors`.
- Elk entreepoortje wordt gesimuleerd door één task.
- Een inkomende bezoeker wordt geregistreerd door de globale variabele `G_NumberOfVisitors` met één te verhogen.
- Zodra een entreepoortje 100.000 bezoekers heeft geteld, laat de bijbehorende task het op dat moment totaal aantal getelde bezoekers zien, dus van alle vier de poortjes samen. Daarna gaat de task oneindig slapen. De task die als laatste klaar is, zou dus een totaal van `4 * 100000 = 400000` bezoekers moeten laten zien.

Omdat alle entreepoortjes volledig identiek functioneren, wordt in dit geval één functie `UserTask` gedefinieerd, waarvan vier kopieën in vier tasks worden gestart. Om deze tasks van elkaar te kunnen onderscheiden, wordt bij `xTaskCreate` een parameter meegegeven: het volgnummer van het entreepoortje, met de waarden 1 tot en met 4. `xTaskCreate` kan bijvoorbeeld als volgt worden aangeroepen:

```cpp
xTaskCreate(UserTask, "tsk_Counter", configMINIMAL_STACK_SIZE * 2,
            (void *)(zooCounter),
            priority, &handle_UserTask);
```

De variabele `zooCounter` is in dit geval van het type `uint32_t` en heeft achtereenvolgens de waarden 1 tot en met 4. In `UserTask` wordt deze parameter weer omgezet naar het volgnummer van het poortje:

```cpp
uint32_t zooEntry = (uint32_t)(pvParameters);
```

Open in Visual Studio Code de folder/map `<path-to-your-project>/workspace/assignments/3_1_zoo_framework`.

:::{warning}
Voeg uitsluitend code toe aan, of wijzig code in, het bestand `src/main.cpp`. Alle overige folders bevatten bestanden voor de systeemconfiguratie en bibliotheken. Laat deze **ongewijzigd**.
:::

:::{admonition} Opdracht 3.1.1
:class: tip
Maak de functie `UserTask` af, zodat deze task de functionaliteit heeft zoals hierboven aangegeven. Vul de code aan op de plaatsen die zijn aangegeven met `TODO`.
:::

:::{admonition} Opdracht 3.1.2
:class: tip
Maak de functie `StartUserTasks` af, zodat hierin vier tasks worden gecreëerd en gestart die elk één entreepoortje simuleren met het daarbij behorende volgnummer (1 t/m 4).
:::

:::{admonition} Opdracht 3.1.3
:class: tip
Run het programma en bekijk het resultaat in de Terminal. Er zouden in totaal `4 x 100000 = 400000` bezoekers moeten zijn geteld. Is dit correct?
:::

De globale variabele `G_NumberOfVisitors` wordt door vier tasks bijgewerkt. Deze tasks lezen en schrijven "tegelijkertijd" dezelfde globale teller. Dat is ongewenst bij deze *shared resource*. De toegang tot deze variabele moet daarom worden afgeschermd, zodat slechts één task tegelijk de variabele kan bijwerken: de variabele lezen, met één verhogen en vervolgens terugschrijven. Hiervoor wordt gebruikgemaakt van een mutex.

:::{admonition} Opdracht 3.1.4
:class: tip

- Declareer een globale handle met de naam `zooMutex` van het type `SemaphoreHandle_t`.
- Creëer de mutex met de functie `xSemaphoreCreateMutex()` in de functie `StartUserTasks`.
- Pas de code in de functie `UserTask` zodanig aan dat de toegang tot de variabele `G_NumberOfVisitors` wordt afgeschermd met deze mutex. Maak hiervoor gebruik van de functies `xSemaphoreTake` en `xSemaphoreGive`.
:::

:::{admonition} Opdracht 3.1.5
:class: tip
Test het aangepaste programma en controleer of het totaal aantal getelde bezoekers nu correct is (namelijk 400000).
:::

## 3.2 Analoog-digitaalconversie met de ADC

Voor het meten van spanningen wordt de ADC-chip (MCP3208) op het NodeMCU-Shield gebruikt. Deze ADC is via de SPI-bus met de ESP32 verbonden en beschikt over 8 analoge ingangen; de ingangsspanning kan op één kanaal tegelijkertijd worden geconverteerd. In de documentatie van de [ADC-bibliotheek](libraries/adc_3208_lib.md) zijn de beschikbare functies beschreven.

Voorlopig heb je alleen de volgende functie nodig. Deze start de conversie, wacht tot deze klaar is en geeft de gemeten spanning in volt terug:

```cpp
double voltage = adc.ReadVoltage(channel);
```

In de volgende opdrachten worden de spanningen gemeten die worden ingesteld met de twee instelpotmeters op het NodeMCU-Shield:

| Potmeter  | ADC-kanaal | Bereik       |
|-----------|------------|--------------|
| RV1 / P0  | 4          | 0 V – 2,5 V  |
| RV2 / P1  | 5          | 0 V – 2,5 V  |

:::{note}
De initialisatie van de `spi`- en `adc`-objecten is al gedaan in de functie `setup()`.
:::

Open in Visual Studio Code de folder/map `<path-to-your-project>/workspace/assignments/3_2_adc_framework`. Voeg ook hier uitsluitend code toe aan, of wijzig code in, het bestand `src/main.cpp`.

:::{admonition} Opdracht 3.2.1
:class: tip
Bekijk de functie `StartUserTasks` en begrijp deze code. Maak vervolgens de functie `TaskADC` af, zodanig dat deze in een oneindige herhaling elke seconde de spanning laat zien van ADC-kanaal 4 (potmeter RV1/P0).

**Aanwijzingen**

- Het kanaalnummer van de ADC is een parameter die in `StartUserTasks` aan `TaskADC` is meegegeven. Maak hiervan gebruik!
- Lees de spanning met `adc.ReadVoltage(channel)` en toon deze met `ts_printf` in de Terminal.
- Controleer of de weergegeven waarden ongeveer correct zijn door aan de potmeter te draaien.
- Gebruik `vTaskDelay(pdMS_TO_TICKS(...))` om de task te laten wachten.
:::

:::{admonition} Opdracht 3.2.2
:class: tip
Breid het programma in `StartUserTasks` uit met een tweede task die dezelfde functie `TaskADC` uitvoert, maar dan voor ADC-kanaal 5 (potmeter RV2/P1). De waarden van beide kanalen worden dus elke seconde in de Terminal weergegeven. Maak dus **geen** kopie van de code van `TaskADC`, maar start een nieuwe task met dezelfde code en een andere parameter!

Controleer of de weergegeven waarden van beide kanalen (ongeveer) correct zijn door aan beide potmeters te draaien.
:::

:::{admonition} Opdracht 3.2.3
:class: tip
Wijzig de functie `TaskADC` zodanig dat ADC-kanaal 4 elke 200 ms wordt weergegeven en ADC-kanaal 5 elke 2 seconden.
:::

De ADC lijkt in deze programma's schijnbaar correct te functioneren en goede waarden te meten. Dit is echter niet gegarandeerd.

:::{admonition} Opdracht 3.2.4
:class: tip
Beredeneer welk principieel probleem optreedt in bovenstaande programma's wanneer twee tasks dezelfde ADC (en dezelfde SPI-bus) gebruiken, en geef aan op welke (bekende) wijze dat kan worden opgelost.
:::

Een andere oplossing voor dit principiële probleem is het toepassen van semaforen. Dit wordt in de volgende opdracht toegepast.

:::{admonition} Opdracht 3.2.5
:class: tip

- Declareer een globale handle met de naam `adcSema` van het type `SemaphoreHandle_t`.
- Creëer de semafoor met de functie `xSemaphoreCreateCounting()` in de functie `StartUserTasks`. De waarde voor zowel de maximale count als de initiële count is 1.
- Pas de code in de functie `TaskADC` zodanig aan dat de toegang tot de ADC is afgeschermd en dat slechts één task tegelijk de ADC kan gebruiken. Maak hiervoor gebruik van de functies `xSemaphoreTake` en `xSemaphoreGive`.
:::

## 3.3 Synchronisatie van tasks

Mutexes worden gebruikt voor mutual exclusion. Ook semaforen kunnen hiervoor worden gebruikt. Een semafoor met maximale waarde 1, ook wel een binaire semafoor genoemd, is functioneel hetzelfde als een mutex. Semaforen hebben echter veel andere toepassingen, bijvoorbeeld het synchroniseren van tasks. In de volgende opdracht worden ze hiervoor gebruikt.

Gegeven zijn twee tasks:

- De task `TaskADC` leest periodiek de spanningen van alle 8 ADC-kanalen (`N_ADC_CHANNELS`) en slaat de gemeten waarden op in de globale array `G_ADCValue[]`.
- De task `TaskDisplay` toont de waarden van `G_ADCValue[]` in de Terminal.

De synchronisatie van deze tasks gebeurt met twee semaforen, zodat de volgorde van het meten van de ingangsspanningen en het weergeven daarvan is gesynchroniseerd: eerst meten, daarna weergeven, enzovoort.

- De task `TaskDisplay` wacht op de semafoor `semaDisplay`. Deze wordt vrijgegeven door `TaskADC`, zodra deze task de waarden van `G_ADCValue[]` heeft bijgewerkt.
- De task `TaskADC` wacht op de semafoor `semaADC` voor een volgende meting. Deze wordt vrijgegeven zodra de task `TaskDisplay` de waarden van `G_ADCValue[]` heeft weergegeven.
- Bij het starten van beide tasks moet `TaskADC` eerst beginnen: eerst een meting uitvoeren en daarna pas de data weergeven. `TaskDisplay` moet wachten op beschikbare data, aangegeven door de semafoor `semaDisplay`. De initiële waarde van `semaDisplay` is daarom 0 en de initiële waarde van `semaADC` is 1.

Open in Visual Studio Code de folder/map `<path-to-your-project>/workspace/assignments/3_3_thread_sync_framework`. Voeg ook hier uitsluitend code toe aan, of wijzig code in, het bestand `src/main.cpp`.

:::{admonition} Opdracht 3.3.1
:class: tip

- Declareer een globale handle met de naam `semaADC` van het type `SemaphoreHandle_t`.
- Declareer een globale handle met de naam `semaDisplay` van het type `SemaphoreHandle_t`.
- Creëer beide semaforen met de functie `xSemaphoreCreateCounting()` in de functie `StartUserTasks`. Gebruik de juiste waarden voor de initiële count en de maximale count!
- Pas de code in de functies `TaskADC` en `TaskDisplay` zodanig aan dat beide tasks met elkaar gesynchroniseerd zijn: `TaskADC` wacht totdat `TaskDisplay` klaar is, en `TaskDisplay` wacht totdat `TaskADC` klaar is, enzovoort. Volg hierbij de `TODO`'s in de code.
- Controleer of de uitvoer in de Terminal correct is en of de volgorde daadwerkelijk is: meten → weergeven → meten → weergeven, enzovoort.
:::
