/*
  CW Paddle Interface for SmartSDR
  ESP32-S3 -> USB MIDI -> SmartSDR

  Paddle:
    TIP    = DIT / left paddle
    RING   = DAH / right paddle
    SLEEVE = GND

  Note mapping:
    Note 20 = DIT / left paddle
    Note 21 = DAH / right paddle

  This version deliberately keeps things simple:
    - no keyer or timing logic on the device
    - no extra bounce guard
    - 150 us edge confirmation
*/

#include <Arduino.h>
#include "USB.h"
#include "USBMIDI.h"


// ------------------------------------------------------------
// USB identity
// ------------------------------------------------------------

// This is the name the device shows up under in the MIDI device
// list. Change it if you like, but remember that SmartSDR stores
// its MIDI assignment per device name, so you have to select the
// device again afterwards.

constexpr char USB_PRODUCT_NAME[] = "Frohrelle CW";
constexpr char USB_MANUFACTURER[] = "Frohrelle";


// ------------------------------------------------------------
// USB MIDI
// ------------------------------------------------------------

USBMIDI MIDI(USB_PRODUCT_NAME);

constexpr uint8_t MIDI_CHANNEL = 1;


// ------------------------------------------------------------
// GPIOs
// ------------------------------------------------------------

constexpr uint8_t PIN_DIT = 4;
constexpr uint8_t PIN_DAH = 5;


// ------------------------------------------------------------
// MIDI notes
// ------------------------------------------------------------

constexpr uint8_t NOTE_DIT = 20;
constexpr uint8_t NOTE_DAH = 21;


// ------------------------------------------------------------
// Debounce
// ------------------------------------------------------------

// A level change has to stay put for at least 150 us before it
// counts as a real paddle edge.

constexpr uint32_t EDGE_CONFIRM_US = 150;


// ------------------------------------------------------------
// Paddle state
// ------------------------------------------------------------

struct PaddleInput
{
    uint8_t pin;
    uint8_t note;

    // last confirmed level
    bool stableState;

    // level currently under observation
    bool candidateState;

    bool candidateActive;

    // when the possible edge started
    uint32_t candidateStartUs;
};


PaddleInput dit;
PaddleInput dah;


// ------------------------------------------------------------
// MIDI event
// ------------------------------------------------------------

struct PaddleEvent
{
    bool valid;
    uint8_t note;
    bool down;
};


// ------------------------------------------------------------
// Set up one paddle input
// ------------------------------------------------------------

void initPaddle(
    PaddleInput &p,
    uint8_t pin,
    uint8_t note
)
{
    p.pin = pin;
    p.note = note;

    // There is already an external 10 kOhm pull-up to 3.3 V.
    pinMode(pin, INPUT);

    bool initialState = digitalRead(pin);

    p.stableState = initialState;
    p.candidateState = initialState;

    p.candidateActive = false;
    p.candidateStartUs = 0;
}


// ------------------------------------------------------------
// Read one paddle input
// ------------------------------------------------------------

PaddleEvent pollPaddle(PaddleInput &p)
{
    PaddleEvent event = {
        false,
        0,
        false
    };

    const uint32_t now = micros();
    const bool raw = digitalRead(p.pin);


    // --------------------------------------------------------
    // No change
    // --------------------------------------------------------

    if (raw == p.stableState)
    {
        // If the apparent edge is gone again, it was just a short
        // glitch or some bounce.
        p.candidateActive = false;

        return event;
    }


    // --------------------------------------------------------
    // Possible new edge
    // --------------------------------------------------------

    if (
        !p.candidateActive ||
        raw != p.candidateState
    )
    {
        p.candidateState = raw;
        p.candidateStartUs = now;
        p.candidateActive = true;

        return event;
    }


    // --------------------------------------------------------
    // Wait until it has been stable for 150 us
    // --------------------------------------------------------

    if (
        (uint32_t)(now - p.candidateStartUs)
        < EDGE_CONFIRM_US
    )
    {
        return event;
    }


    // --------------------------------------------------------
    // Accept the edge
    // --------------------------------------------------------

    p.stableState = p.candidateState;
    p.candidateActive = false;

    event.valid = true;
    event.note = p.note;

    // Because of the pull-up:
    //
    // LOW  = paddle pressed
    // HIGH = paddle released

    event.down = (p.stableState == LOW);

    return event;
}


// ------------------------------------------------------------
// Send a MIDI event
// ------------------------------------------------------------

void sendMidiEvent(const PaddleEvent &event)
{
    if (!event.valid)
    {
        return;
    }

    if (event.down)
    {
        MIDI.noteOn(
            event.note,
            127,
            MIDI_CHANNEL
        );
    }
    else
    {
        MIDI.noteOff(
            event.note,
            0,
            MIDI_CHANNEL
        );
    }
}


// ------------------------------------------------------------
// Setup
// ------------------------------------------------------------

void setup()
{
    // Paddle inputs

    initPaddle(
        dit,
        PIN_DIT,
        NOTE_DIT
    );

    initPaddle(
        dah,
        PIN_DAH,
        NOTE_DAH
    );


    // USB descriptor strings

    USB.productName(USB_PRODUCT_NAME);
    USB.manufacturerName(USB_MANUFACTURER);


    // Start MIDI and USB

    MIDI.begin();
    USB.begin();
}


// ------------------------------------------------------------
// Main loop
// ------------------------------------------------------------

void loop()
{
    // DIT

    PaddleEvent ditEvent = pollPaddle(dit);

    if (ditEvent.valid)
    {
        sendMidiEvent(ditEvent);
    }


    // DAH

    PaddleEvent dahEvent = pollPaddle(dah);

    if (dahEvent.valid)
    {
        sendMidiEvent(dahEvent);
    }
}
