# Hardware

Zwei identische Eingangskanäle, einer für DIT (TIP), einer für DAH (RING).
Sleeve liegt gemeinsam auf Masse.

![Schaltplan](schematic.svg)

## Schaltplan (Textform)

```
3,5-mm-TRS                      ESP32-S3

TIP ---- 1 kΩ -------+-------- GPIO4  (DIT)
                     |
                     +-- 10 kΩ -- 3,3 V
                     |
                     +-- 1 nF --- GND


RING --- 1 kΩ -------+-------- GPIO5  (DAH)
                     |
                     +-- 10 kΩ -- 3,3 V
                     |
                     +-- 1 nF --- GND


SLEEVE ---------------------- GND
```

## Stückliste

| Menge | Bauteil                          | Bemerkung                                    |
|------:|----------------------------------|----------------------------------------------|
|     1 | ESP32-S3-Board mit nativem USB   | z. B. ESP32-S3-DevKitC-1, USB-C am S3 (nicht am UART-Chip) |
|     1 | 3,5-mm-Klinkenbuchse, stereo     | TRS, für das Paddle-Kabel                     |
|     2 | Widerstand 1 kΩ                  | Serienwiderstand in TIP und RING              |
|     2 | Widerstand 10 kΩ                 | Pull-up nach 3,3 V                            |
|     2 | Kondensator 1 nF                 | nach GND, direkt am Pin                       |
|     1 | Gehäuse                          | beliebig, Buchse und USB-Anschluss herausführen |

## Warum diese Bauteile

**10 kΩ Pull-up nach 3,3 V** – der Eingang liegt im Ruhezustand sauber auf HIGH.
Schließt der Paddle-Kontakt gegen Sleeve/GND, geht der Pin auf LOW. Der externe
Pull-up wird bewusst verwendet, statt sich auf den internen Pull-up des Reglers zu
verlassen: er ist definiert niederohmig genug, um an einem längeren, offen
liegenden Paddlekabel HF-fest zu bleiben. Im Sketch ist der Pin deshalb als
`INPUT` (ohne internen Pull-up) konfiguriert.

**1 kΩ in Reihe** – begrenzt den Strom in die Schutzdioden des Controllers, wenn
am Kabel statische Entladung oder HF-Einstreuung ankommt. Der Spannungsteiler
1 kΩ / 10 kΩ liefert im geschlossenen Zustand rund 0,3 V am Pin, also sicher
unter der LOW-Schwelle.

**1 nF nach GND** – bildet mit den Widerständen einen Tiefpass. Fallende Flanke
(Kontakt schließt) mit τ ≈ 1 µs, steigende Flanke (Kontakt öffnet) mit
τ ≈ 10 µs. Das ist um Größenordnungen schneller als jedes Tastzeichen, dämpft
aber HF aus der eigenen Endstufe und die schnellsten Kontaktprellungen. Der
Kondensator gehört so dicht wie möglich an den GPIO-Pin.

## Aufbauhinweise

- Das USB-Kabel am **nativen** USB-Port des ESP32-S3 anschließen. Der zweite Port
  vieler Boards hängt an einem UART-Brücken-IC und kann kein USB-MIDI.
- Paddlekabel kurz halten und die Masse der Buchse sternförmig auf die Board-Masse
  führen.
- Wer bei hoher Leistung arbeitet: Ferritkern über das Paddlekabel und über das
  USB-Kabel dicht am Gehäuse.
- Bei einem Single-Lever-Paddle oder einer Handtaste nur TIP belegen; RING bleibt
  über den Pull-up auf HIGH und sendet nie.
