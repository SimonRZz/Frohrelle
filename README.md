# Frohrelle

**A CW paddle to USB MIDI interface for SmartSDR.**

A homebrew CW paddle interface. An ESP32-S3 registers itself on the computer as
a USB MIDI device and reports both paddle contacts as MIDI notes. SmartSDR reads
them as dit and dah and passes them to the keyer in the radio.

You need an ESP32-S3 board with native USB, a 3.5 mm stereo jack and a handful of
passive parts.

## Why bother

With SmartSDR the operator and the radio are often not in the same room, so the
paddle has to plug into the PC that runs SmartSDR rather than into the
transceiver. That is the gap this little box fills.

A few things it gets right:

* USB MIDI is a standard device class. Windows, macOS and Linux pick the
  interface up on their own. Nothing to install, no driver, no COM port juggling,
  no second power supply.
* The interface only ever reports "contact closed" and "contact open". Speed,
  weighting, iambic mode, message memories and above all the sidetone stay in the
  radio. The sidetone therefore does not wobble the way it does when a PC forms
  the characters itself.
* Events go out the moment an edge is confirmed. Nothing is buffered up, and the
  debounce works in microseconds rather than milliseconds.
* You key with your own paddle, iambic, exactly as you are used to, instead of
  reaching for keyboard macros during a pile-up.
* Every part is standard stock. The firmware is a single Arduino sketch, and the
  pins, notes, channel, debounce time and device name are all constants at the
  top of it.

It works equally well as a permanent fixture at the station and as something you
throw in a rucksack: one board, one cable, and nothing to set up on whatever
computer you plug it into.

## How it works

```
   Paddle          Interface                       Computer           Transceiver
  +--------+     +--------------+               +----------+        +-----------+
  |  DIT   |-----| GPIO4        |   USB MIDI    |          |  LAN   |  keyer,   |
  |  DAH   |-----| GPIO5  ESP32 |-------------->| SmartSDR |------->|  sidetone |
  |  GND   |-----| GND      -S3 |  note 20/21   |          |        |  TX       |
  +--------+     +--------------+               +----------+        +-----------+
```

Both paddle contacts sit at 3.3 V through a 10 kOhm pull-up and are pulled to
ground when you press the paddle. The firmware polls both pins in the main loop:

| Action          | Pin level | MIDI message                        |
|-----------------|-----------|-------------------------------------|
| DIT pressed     | LOW       | `Note On` 20, velocity 127, channel 1 |
| DIT released    | HIGH      | `Note Off` 20, velocity 0, channel 1  |
| DAH pressed     | LOW       | `Note On` 21, velocity 127, channel 1 |
| DAH released    | HIGH      | `Note Off` 21, velocity 0, channel 1  |

A level change is only sent once it has held for **150 us**. Anything shorter,
contact bounce or RF pickup, gets thrown away, and real edges are not delayed in
any way you could notice. At 40 WPM a dit lasts roughly 30 ms, so the debounce
window is about 0.5 % of a dot.

## Connections

3.5 mm stereo (TRS) plug:

| Contact | Function           | GPIO |
|---------|--------------------|------|
| Tip     | DIT / left paddle  | 4    |
| Ring    | DAH / right paddle | 5    |
| Sleeve  | ground             | GND  |

## Building it

Schematic, bill of materials and the reasoning behind each part are in
[hardware/](hardware/).

The short version, once per contact: 1 kOhm in series with the GPIO, 10 kOhm as a
pull-up to 3.3 V, 1 nF to ground right at the pin. Sleeve goes straight to GND.

## Firmware

The sketch, flashing instructions and the knobs you can turn are in
[firmware/](firmware/).

Two things matter when flashing. The ESP32-S3 has to be built in **USB-OTG
(TinyUSB)** mode, otherwise the USB MIDI class is not available at all. And the
cable belongs on the native USB port of the S3, not on the UART bridge port that
many boards also carry.

## Setting it up in SmartSDR

1. Plug the interface in. It appears as a MIDI input on the computer.
2. In SmartSDR, select or enable the MIDI device and point it at the slice you
   want to key.
3. Switch to CW, then set keyer speed and iambic mode in the radio as usual and
   pick your break-in setting.
4. Test it with the PA off first. The sidetone should start while the paddle is
   closed and stop the instant you let go.

If you want to know whether the notes leave the interface at all, any MIDI
monitor will tell you, independently of SmartSDR: pressing should produce
`Note On 20` or `21`, releasing the matching `Note Off`.

## Troubleshooting

| Symptom                                    | Cause and cure                                                                  |
|--------------------------------------------|---------------------------------------------------------------------------------|
| No MIDI device shows up                    | Cable on the UART port instead of the native USB port, or firmware built without TinyUSB mode |
| Continuous tone right after plugging in    | Paddle contact closed, sleeve not on ground, or a missing pull-up                |
| Dit and dah swapped                        | Swap `PIN_DIT` and `PIN_DAH` in the sketch, or use the paddle reverse setting in software |
| Characters only break up while transmitting | RF getting in: ferrite on the paddle lead and the USB cable, and keep the 1 nF caps close to the pins |
| Occasional doubled elements                | Bouncy paddle contacts: raise `EDGE_CONFIRM_US` step by step, 500 us is a good next try |
| Compiler does not know `USBMIDI`           | The *USB Mode* board setting is not on *USB-OTG (TinyUSB)*                       |

## Repository layout

```
firmware/CWPaddleMIDI/CWPaddleMIDI.ino   Arduino sketch
firmware/README.md                       flashing and customising
hardware/schematic.svg                   schematic
hardware/README.md                       bill of materials, part by part reasoning
platformio.ini                           build configuration for PlatformIO
```

## Status

Working version, in use. The firmware deliberately does no timing work and has no
extra bounce guard beyond the 150 us edge confirmation and the RC network on the
input.

## License

Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0), see
[LICENSE](LICENSE). Build it, modify it, pass it on, just do not sell it and do
not put it into something you sell.
