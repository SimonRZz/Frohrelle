# Firmware

`FrohrelleKey/FrohrelleKey.ino`, an Arduino sketch for the ESP32-S3 with native
USB.

## Arduino IDE

1. Install the **esp32 by Espressif Systems** board package, version 3.x.
2. Board: *ESP32S3 Dev Module*, or whatever your board actually is.
3. Settings that matter, under *Tools*:
   * **USB Mode: USB-OTG (TinyUSB)**, required, otherwise `USBMIDI` does not exist.
   * **USB CDC On Boot: Enabled**, convenient so the port survives flashing.
   * **Upload Mode: UART0 / Hardware CDC**
4. Upload. If the board stops showing up as a serial port afterwards: hold BOOT,
   tap RESET, let go, then flash again.

## PlatformIO

`platformio.ini` in the repository root already carries the build flags you need
and points `src_dir` at `firmware/FrohrelleKey/`.

```
pio run -t upload
```

## What the sketch does

* Polls GPIO4 (DIT) and GPIO5 (DAH) in the main loop. No interrupts.
* Accepts a level change only after it has held for **150 us**
  (`EDGE_CONFIRM_US`). If the level moves again inside that window, the candidate
  edge is dropped.
* Turns accepted edges straight into MIDI events:
  * LOW (pressed) sends `Note On`, velocity 127
  * HIGH (released) sends `Note Off`, velocity 0
* Notes: **20 = DIT**, **21 = DAH**, MIDI channel **1**.

There is deliberately no keyer, no timing and no message memory on the device. It
reports "contact closed" and "contact open" and nothing else. The timing is the
job of the keyer in the transceiver.

WiFi and Bluetooth are never started, which keeps the current draw low enough to
run off a phone. Flash the board from a computer, then move it over to the phone
or tablet you actually operate with.

## Things you can change

| Constant in the sketch | What it does                                |
|------------------------|---------------------------------------------|
| `PIN_DIT`, `PIN_DAH`   | GPIO assignment                             |
| `NOTE_DIT`, `NOTE_DAH` | MIDI note numbers                           |
| `MIDI_CHANNEL`         | MIDI channel                                |
| `EDGE_CONFIRM_US`      | debounce time in microseconds               |
| `USB_PRODUCT_NAME`     | name the device appears under in the MIDI list |
| `USB_MANUFACTURER`     | manufacturer string in the USB descriptor   |

Changing `USB_PRODUCT_NAME` means SmartSDR sees a different device, so you have
to select it again in the MIDI settings.

Paddle the wrong way round? Either swap `PIN_DIT` and `PIN_DAH`, or use the
paddle reverse setting in your station software.
