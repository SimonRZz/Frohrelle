# Frohrelle — CW-Paddle-Interface über USB-MIDI

Ein kleines Selbstbau-Interface, das ein klassisches CW-Paddle über USB an den
Rechner bringt. Das Paddle wird als **USB-MIDI-Gerät** angemeldet: Jedes
Schließen und Öffnen eines Paddle-Kontakts erzeugt sofort ein MIDI-Event, das
SmartSDR als Dit bzw. Dah auswertet und an den Keyer im Transceiver weitergibt.

Hardware: ein ESP32-S3 mit nativem USB, eine 3,5-mm-Klinkenbuchse und eine
Handvoll passiver Bauteile.

---

## Wozu das gut ist

Beim netzwerkbasierten Betrieb (SmartSDR und Transceiver getrennt, oft über LAN
oder Remote) liegt das Paddle dort, wo der Operator sitzt — nicht am Gerät. Genau
diese Lücke schließt das Interface:

- **Kein zusätzliches Keyer-Kabel.** Das Paddle hängt an demselben Rechner, auf
  dem SmartSDR läuft. Kein serieller Adapter, kein extra Netzteil.
- **Keine Treiber.** USB-MIDI ist eine Standard-Geräteklasse. Windows, macOS und
  Linux erkennen das Interface sofort, ohne Installation und ohne COM-Port-Gerangel.
- **Das Timing bleibt im Transceiver.** Das Interface überträgt nur "Kontakt zu"
  und "Kontakt auf". Gewichtung, Geschwindigkeit, Iambic A/B, Zeichenspeicher und
  vor allem der Sidetone werden weiterhin vom Keyer im Radio erzeugt. Damit bleibt
  das Mithörzeichen frei von den Schwankungen, die entstehen, wenn ein PC das
  Zeichen selbst formt.
- **Sofortige Reaktion.** MIDI-Events werden ohne Sammelpuffer gesendet, sobald
  eine Flanke bestätigt ist. Die Entprellung wirkt in Mikrosekunden, nicht in
  Millisekunden.
- **Vollwertiges Paddle statt Tastatur.** Iambic-Betrieb mit dem gewohnten eigenen
  Paddle, auch beim Contest und beim Pile-Up, statt Makro-Tasten oder Winkeyer-Ersatz.
- **Nachbaubar und offen.** Alle Bauteile sind Standardware, die Firmware ist ein
  einziger Arduino-Sketch, alle Parameter (Pins, Noten, Kanal, Entprellzeit,
  Gerätename) stehen als Konstanten oben im Code.

Der Aufbau eignet sich als kleines Dauer-Interface an der Station genauso wie als
Portabellösung im Rucksack: ein Board, ein Kabel, kein Setup am Zielrechner.

---

## Funktionsprinzip

```
   Paddle          Interface                       Rechner            Transceiver
  ┌────────┐     ┌──────────────┐               ┌──────────┐        ┌───────────┐
  │  DIT   ├─────┤ GPIO4        │   USB-MIDI    │          │  LAN   │  Keyer,   │
  │  DAH   ├─────┤ GPIO5  ESP32 ├──────────────►│ SmartSDR ├───────►│  Sidetone │
  │  GND   ├─────┤ GND      -S3 │  Note 20/21   │          │        │  TX       │
  └────────┘     └──────────────┘               └──────────┘        └───────────┘
```

Beide Paddle-Kontakte liegen über einen 10-kΩ-Pull-up an 3,3 V und werden beim
Drücken gegen Masse gezogen. Die Firmware pollt beide Pins in der Hauptschleife:

| Vorgang            | Pegel am Pin | MIDI-Nachricht                    |
|--------------------|--------------|-----------------------------------|
| DIT gedrückt       | LOW          | `Note On` 20, Velocity 127, Kan. 1 |
| DIT losgelassen    | HIGH         | `Note Off` 20, Velocity 0, Kan. 1  |
| DAH gedrückt       | LOW          | `Note On` 21, Velocity 127, Kan. 1 |
| DAH losgelassen    | HIGH         | `Note Off` 21, Velocity 0, Kan. 1  |

Eine Pegeländerung wird erst gesendet, wenn sie **150 µs** ununterbrochen anliegt.
Kürzeres Zappeln — Kontaktprellen, HF-Einstreuung — wird verworfen, ohne dass
echte Flanken merklich verzögert werden. Bei 40 WpM dauert ein Dit rund 30 ms,
die Entprellzeit liegt also bei etwa 0,5 % eines Punktes.

---

## Belegung

**Klinkenstecker 3,5 mm (TRS)**

| Kontakt | Funktion            | GPIO |
|---------|---------------------|------|
| Tip     | DIT / linkes Paddle | 4    |
| Ring    | DAH / rechtes Paddle| 5    |
| Sleeve  | Masse               | GND  |

---

## Aufbau

Schaltplan, Stückliste und die Begründung für jedes Bauteil: **[hardware/](hardware/)**

Kurzfassung je Kontakt: 1 kΩ in Reihe zum GPIO, 10 kΩ als Pull-up nach 3,3 V,
1 nF nach GND direkt am Pin. Sleeve geht ohne Umweg auf GND.

---

## Firmware

Sketch, Flash-Anleitung und Einstellmöglichkeiten: **[firmware/](firmware/)**

Wichtig beim Flashen: Der ESP32-S3 muss im Modus **USB-OTG (TinyUSB)** gebaut
werden, sonst steht die USB-MIDI-Klasse nicht zur Verfügung. Das USB-Kabel gehört
an den nativen USB-Port des S3, nicht an den UART-Brücken-Port.

---

## Einrichtung in SmartSDR

1. Interface anstecken. Es meldet sich als MIDI-Eingang am Rechner an.
2. In SmartSDR das MIDI-Gerät auswählen bzw. aktivieren und dem Slice zuordnen,
   der getastet werden soll.
3. CW-Modus wählen, Keyer-Geschwindigkeit und Iambic-Modus wie gewohnt im
   Radio einstellen, Break-In nach Geschmack.
4. Funktionstest zunächst ohne Sendeleistung: Bei gedrücktem Paddle muss der
   Sidetone stehen, beim Loslassen sofort aufhören.

Ob die Zeichen überhaupt ankommen, lässt sich unabhängig von SmartSDR mit einem
beliebigen MIDI-Monitor prüfen: Beim Drücken muss `Note On 20` bzw. `21`
erscheinen, beim Loslassen das passende `Note Off`.

---

## Fehlersuche

| Symptom                                   | Ursache / Abhilfe                                                                 |
|-------------------------------------------|-----------------------------------------------------------------------------------|
| Kein MIDI-Gerät am Rechner                | Kabel am UART-Port statt am nativen USB-Port; oder Build ohne TinyUSB-Modus        |
| Dauerton / Dauerzeichen direkt nach dem Anstecken | Paddle-Kontakt geschlossen, Sleeve nicht auf GND, oder Pull-up fehlt        |
| Dit und Dah vertauscht                    | `PIN_DIT` / `PIN_DAH` im Sketch tauschen oder Paddle-Umkehr in der Software nutzen |
| Zeichen stottern nur beim Senden          | HF-Einstreuung: Ferrit auf Paddle- und USB-Kabel, 1-nF-Kondensatoren dicht an die Pins |
| Gelegentliche Doppelzeichen               | Prellende Paddle-Kontakte: `EDGE_CONFIRM_US` schrittweise erhöhen (z. B. 500 µs)    |
| Compilerfehler `USBMIDI` unbekannt        | Board-Einstellung *USB Mode* steht nicht auf *USB-OTG (TinyUSB)*                   |

---

## Projektstruktur

```
firmware/CWPaddleMIDI/CWPaddleMIDI.ino   Arduino-Sketch
firmware/README.md                       Flashen und Anpassen
hardware/schematic.svg                   Schaltplan
hardware/README.md                       Stückliste und Bauteilbegründung
platformio.ini                           Build-Konfiguration für PlatformIO
```

---

## Status

Funktionsfähige Arbeitsversion. Die Firmware arbeitet bewusst ohne
Timing-Aufbereitung und ohne zusätzlichen Bounce-Guard; die Entprellung besteht
aus der 150-µs-Flankenbestätigung und dem RC-Glied am Eingang.
