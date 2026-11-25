# Opdracht 1: Installatie van de software

In de workshop Realtime Software Engineering wordt voor de hardware gebruikgemaakt van een ESP32 (NodeMCU-32S) in combinatie met het NodeMCU-Shield. De te gebruiken software bestaat uit de volgende onderdelen:

1. **Visual Studio Code**: de editor waarin de code wordt geschreven.
2. **PlatformIO**: de extensie voor Visual Studio Code waarmee de code wordt gecompileerd en naar de ESP32 wordt geüpload.
3. **Terminal/Serial Monitor van PlatformIO**: voor de weergave van de tekstuele programma-uitvoer van de ESP32 en voor het invoeren van commando's.

## 1.1 Installeren van de ontwikkelomgeving

:::{admonition} Opdracht 1.1.1
:class: tip
Installeer Visual Studio Code en de PlatformIO-extensie, en haal de workshopbestanden op. Volg hiervoor de [installatie-instructies](install.md).
:::

## 1.2 Testen van de geïnstalleerde ontwikkelomgeving

:::{admonition} Opdracht 1.2.1
:class: tip
Test de ontwikkelomgeving met het project `<path-to-your-project>/workspace/examples/installatie_validatie`, zoals beschreven in [Testen van de installatie](install.md#testen-van-de-installatie):

- Compileer en upload het programma naar de ESP32. Controleer of er geen compilatie- of linkfouten (errors) zijn. Warnings mogen voorlopig worden genegeerd.
- Controleer of de `heartbeat`-LED op het ESP32-board knippert. Deze LED wordt in alle volgende practica op dezelfde manier gebruikt: hij geeft aan dat FreeRTOS op de ESP32 actief is. Als deze LED niet knippert, is er dus iets mis.
:::

:::{admonition} Opdracht 1.2.2
:class: tip
Open de Terminal en druk op de `RESET`-knop van het board. De ESP32 start opnieuw op en in de Terminal verschijnt de opstartmelding met de lijst van beschikbare commando's (zie [Gebruik van de monitor & terminal/console](install.md#gebruik-van-de-monitor--terminalconsole)).

Probeer de commando's `help`, `version`, `cpuinfo`, `memory` en `tasks` uit door deze in de Terminal in te voeren.
:::

Als bovenstaande punten allemaal correct functioneren, is de ontwikkelomgeving correct geïnstalleerd en kunnen de volgende practica worden uitgevoerd.
