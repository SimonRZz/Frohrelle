# Firmware

`CWPaddleMIDI/CWPaddleMIDI.ino` – Arduino-Sketch für ESP32-S3 mit nativem USB.

## Arduino IDE

1. Boardpaket **esp32 by Espressif Systems** installieren (Version 3.x).
2. Board: *ESP32S3 Dev Module* (oder das konkrete Board).
3. Wichtige Einstellungen im Menü *Tools*:
   - **USB Mode: USB-OTG (TinyUSB)** – zwingend, sonst existiert `USBMIDI` nicht.
   - **USB CDC On Boot: Enabled** – bequem, damit der Port nach dem Flashen bleibt.
   - **Upload Mode: UART0 / Hardware CDC**
4. Hochladen. Reagiert das Board nach dem Flashen nicht mehr als serieller Port:
   BOOT gedrückt halten, RESET tippen, loslassen – dann erneut flashen.

## PlatformIO

Die mitgelieferte `platformio.ini` setzt die nötigen Build-Flags bereits.
Der Sketch liegt unter `firmware/CWPaddleMIDI/`; PlatformIO ist in der
`platformio.ini` auf dieses Verzeichnis als `src_dir` eingestellt.

```
pio run -t upload
```

## Was der Sketch tut

- Pollt GPIO4 (DIT) und GPIO5 (DAH) in der Hauptschleife, ohne Interrupts.
- Eine Pegeländerung wird erst akzeptiert, wenn sie **150 µs** stabil anliegt
  (`EDGE_CONFIRM_US`). Zappelt der Pegel in dieser Zeit, verfällt der Kandidat.
- Akzeptierte Flanken erzeugen sofort MIDI-Events:
  - LOW (gedrückt) → `Note On`, Velocity 127
  - HIGH (losgelassen) → `Note Off`, Velocity 0
- Noten: **20 = DIT**, **21 = DAH**, MIDI-Kanal **1**.

Es wird bewusst kein Keyer, kein Timing und kein Zeichenspeicher in der Firmware
gerechnet. Das Interface meldet nur "Kontakt zu" und "Kontakt auf" – das Timing
macht der Keyer im Transceiver.

## Anpassen

| Stelle im Sketch            | Zweck                                             |
|-----------------------------|---------------------------------------------------|
| `PIN_DIT`, `PIN_DAH`        | GPIO-Zuordnung                                    |
| `NOTE_DIT`, `NOTE_DAH`      | MIDI-Notennummern                                 |
| `MIDI_CHANNEL`              | MIDI-Kanal                                        |
| `EDGE_CONFIRM_US`           | Entprellzeit in Mikrosekunden                     |
| `USB.productName(...)`      | Name, unter dem das Gerät in der MIDI-Liste steht |
| `USB.manufacturerName(...)` | Herstellerstring im USB-Deskriptor                |

Paddle seitenverkehrt? Entweder `PIN_DIT` und `PIN_DAH` tauschen oder die
Paddle-Umkehr in der Station-Software nutzen.
