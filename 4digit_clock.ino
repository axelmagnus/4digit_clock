// Simple segment/digit test: step through each segment on each digit, 200 ms per step.

#include <Arduino.h>

// ESP32 Huzzah32 bare 4-digit 7-seg test
// Wiring (example):
//   Segments a,b,c,d,e,f,g,dp -> GPIOs below, no resistors
//   Digit cathodes D1..D4 -> each through 150–220 Ω to the GPIOs below
//   Cathodes sink to GND when LOW
// Use one segment at a time to keep current bounded by the cathode resistor.

const int segPins[7] = {21, 17, 16, 19, 18, 5, 4}; // a,b,c,d,e,f,g (dp unused)
const int numSegPins = 7;
const int digitPins[4] = {27, 33, 15, 32}; // D1..D4 cathodes
const int colonPins[1] = {23};             // colon anode
const int colonCathodes[1] = {22};         // colon cathode

const unsigned int stepDelayMs = 200; // dwell per step

void allOff()
{
    // turn off all segments and colon
    for (int s = 0; s < numSegPins; s++)
    {
        digitalWrite(segPins[s], LOW);
    }
    for (int d = 0; d < 4; d++)
    {
        digitalWrite(digitPins[d], HIGH); // HIGH = digit off (not sinking)
    }
    digitalWrite(colonPins[0], LOW);
    digitalWrite(colonCathodes[0], HIGH);
}

void setup()
{
    Serial.begin(115200);
    for (int p : segPins)
    {
        pinMode(p, OUTPUT);
        digitalWrite(p, LOW);
    }
    for (int p : digitPins)
    {
        pinMode(p, OUTPUT);
        digitalWrite(p, HIGH); // off
    }
    pinMode(colonPins[0], OUTPUT);
    digitalWrite(colonPins[0], LOW);
    pinMode(colonCathodes[0], OUTPUT);
    digitalWrite(colonCathodes[0], HIGH);

    allOff();
}

void loop()
{
    // walk each digit and light each segment one by one
    for (int d = 0; d < 4; d++)
    {
        for (int s = 0; s < numSegPins; s++)
        {
            allOff();
            digitalWrite(digitPins[d], LOW); // enable this digit
            digitalWrite(segPins[s], HIGH);  // light one segment
            // print segment and digit to serial on one line
            Serial.print("Digit: ");
            Serial.print(d);
            Serial.print(", Segment: ");
            Serial.println(s);
            delay(stepDelayMs);
        }
    }

    // test the colon separately
    allOff();
    digitalWrite(colonCathodes[0], LOW);
    digitalWrite(colonPins[0], HIGH);
    delay(stepDelayMs);
}
