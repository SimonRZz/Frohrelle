/*
  SmartSDR CW Paddle Interface
  ESP32-S3 -> USB MIDI -> SmartSDR

  Paddle:
    TIP    = DIT / Left Paddle
    RING   = DAH / Right Paddle
    SLEEVE = GND

  SmartSDR / HaliKey Mapping:
    Note 20 = DIT / Left Paddle
    Note 21 = DAH / Right Paddle

  Diese Testversion verwendet:
    - kein MoMIDI-Timing
    - keinen Bounce-Guard
    - 150 us Flankenbestaetigung
*/

#include <Arduino.h>
#include "USB.h"
#include "USBMIDI.h"


// ------------------------------------------------------------
// USB MIDI
// ------------------------------------------------------------

USBMIDI MIDI("HaliKey MIDI");

constexpr uint8_t MIDI_CHANNEL = 1;


// ------------------------------------------------------------
// GPIOs
// ------------------------------------------------------------

constexpr uint8_t PIN_DIT = 4;
constexpr uint8_t PIN_DAH = 5;


// ------------------------------------------------------------
// MIDI Notes wie beim HaliKey
// ------------------------------------------------------------

constexpr uint8_t NOTE_DIT = 20;
constexpr uint8_t NOTE_DAH = 21;


// ------------------------------------------------------------
// Debounce
// ------------------------------------------------------------

// Eine Zustandsaenderung muss mindestens 150 us stabil sein,
// bevor sie als echte Paddle-Flanke akzeptiert wird.

constexpr uint32_t EDGE_CONFIRM_US = 150;


// ------------------------------------------------------------
// Paddle-Zustand
// ------------------------------------------------------------

struct PaddleInput
{
    uint8_t pin;
    uint8_t note;

    // letzter bestaetigter Zustand
    bool stableState;

    // momentan beobachteter moeglicher neuer Zustand
    bool candidateState;

    bool candidateActive;

    // Zeitpunkt des Beginns der moeglichen Flanke
    uint32_t candidateStartUs;
};


PaddleInput dit;
PaddleInput dah;


// ------------------------------------------------------------
// MIDI Event
// ------------------------------------------------------------

struct PaddleEvent
{
    bool valid;
    uint8_t note;
    bool down;
};


// ------------------------------------------------------------
// Paddle initialisieren
// ------------------------------------------------------------

void initPaddle(
    PaddleInput &p,
    uint8_t pin,
    uint8_t note
)
{
    p.pin = pin;
    p.note = note;

    // Wir haben extern bereits 10 kOhm Pull-up nach 3,3 V.
    pinMode(pin, INPUT);

    bool initialState = digitalRead(pin);

    p.stableState = initialState;
    p.candidateState = initialState;

    p.candidateActive = false;
    p.candidateStartUs = 0;
}


// ------------------------------------------------------------
// Paddle-Eingang abfragen
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
    // Keine Aenderung
    // --------------------------------------------------------

    if (raw == p.stableState)
    {
        // Falls die vermeintliche Flanke wieder verschwunden ist,
        // war sie nur eine kurze Stoerung / Bounce.
        p.candidateActive = false;

        return event;
    }


    // --------------------------------------------------------
    // Neue moegliche Flanke
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
    // Warten, bis sie 150 us stabil ist
    // --------------------------------------------------------

    if (
        (uint32_t)(now - p.candidateStartUs)
        < EDGE_CONFIRM_US
    )
    {
        return event;
    }


    // --------------------------------------------------------
    // Flanke akzeptieren
    // --------------------------------------------------------

    p.stableState = p.candidateState;
    p.candidateActive = false;

    event.valid = true;
    event.note = p.note;

    // Wegen des Pull-ups:
    //
    // LOW  = Paddle gedrueckt
    // HIGH = Paddle losgelassen

    event.down = (p.stableState == LOW);

    return event;
}


// ------------------------------------------------------------
// MIDI Event senden
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
    // Paddle-Eingaenge initialisieren

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


    // USB-Bezeichnung

    USB.productName("HaliKey MIDI");
    USB.manufacturerName("DIY HaliKey");


    // MIDI und USB starten

    MIDI.begin();
    USB.begin();
}


// ------------------------------------------------------------
// Hauptschleife
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
