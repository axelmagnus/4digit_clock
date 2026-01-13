// 4-digit clock using NTP + timezone. Colon toggles once per minute.

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include "config.h" // defines WIFI_SSID, WIFI_PASS

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

// bit order: 0=a,1=b,2=c,3=d,4=e,5=f,6=g,7=dp
const byte digitLUT[10] = {
    B00111111, // 0
    B00000110, // 1
    B01011011, // 2
    B01001111, // 3
    B01100110, // 4
    B01101101, // 5
    B01111101, // 6
    B00000111, // 7
    B01111111, // 8
    B01101111  // 9
};

byte frame[4] = {0, 0, 0, 0};             // per-digit segment masks
const unsigned int segmentPulseUs = 1000; // on-time per lit segment; raise for brightness
const int scanRepeats = 4;                // how many full multiplex passes per loop for brightness
bool colonState = false;                  // toggled when a time message is received
int lastDisplayedValue = -1;              // track HHMM to avoid redundant updates
int lastMinute = -1;                      // last minute shown to decide updates
unsigned long lastBlinkMs = 0;            // colon blink timer

void updateFromRTC();
void showNumber(int value);
void scanOnceOneSegmentAtATime();
void connectWiFi();

void setup()
{
    for (int p : segPins)
    {
        pinMode(p, OUTPUT);
        digitalWrite(p, LOW);
    }
    for (int p : digitPins)
    {
        pinMode(p, OUTPUT);
        digitalWrite(p, HIGH);
    } // HIGH = off (not sinking)
    for (int p : colonPins)
    {
        pinMode(p, OUTPUT);
        digitalWrite(p, LOW);
    }
    for (int p : colonCathodes)
    {
        pinMode(p, OUTPUT);
        digitalWrite(p, HIGH);
    }

    connectWiFi();

    // Configure timezone for Europe/Stockholm (CET/CEST with DST rules)
    configTzTime("CET-1CEST,M3.5.0/2,M10.5.0/3", "pool.ntp.org", "time.nist.gov");

    showNumber(0); // start at 0000
}

void loop()
{
    updateFromRTC();

    // Refresh the display a few times per loop for steadier brightness
    for (int i = 0; i < scanRepeats; i++)
    {
        scanOnceOneSegmentAtATime();
    }

    // drive colon pins directly based on colonState
    digitalWrite(colonCathodes[0], colonState ? LOW : HIGH);
    digitalWrite(colonPins[0], colonState ? HIGH : LOW);

    // blink colon once per second regardless of minute updates
    unsigned long nowMs = millis();
    if (nowMs - lastBlinkMs >= 1000)
    {
        lastBlinkMs = nowMs;
        colonState = !colonState;
    }
}

// Update frame[] with a 4-digit number; leading zeros shown
void showNumber(int value)
{
    int v = value % 10000;
    frame[3] = digitLUT[v % 10];
    frame[2] = digitLUT[(v / 10) % 10];
    frame[1] = digitLUT[(v / 100) % 10];
    frame[0] = digitLUT[(v / 1000) % 10];
}

// Poll RTC (NTP-set) and update display only when minute changes
void updateFromRTC()
{
    static unsigned long lastPollMs = 0;
    unsigned long nowMs = millis();
    if (nowMs - lastPollMs < 500)
    {
        return; // limit polling
    }
    lastPollMs = nowMs;

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        return; // time not yet set
    }

    int minute = timeinfo.tm_min;
    if (minute == lastMinute)
    {
        return; // no minute change
    }
    lastMinute = minute;

    int hours = timeinfo.tm_hour;
    int mins = timeinfo.tm_min;
    int value = (hours % 24) * 100 + (mins % 60); // HHMM

    colonState = !colonState; // blink colon once per new minute
    lastDisplayedValue = value;
    showNumber(value);
}

// Multiplex: for each digit, light only the segments it needs, one at a time.
void scanOnceOneSegmentAtATime()
{
    for (int d = 0; d < 4; d++)
    {
        byte mask = frame[d]; // single byte: bits 0-7 map to a,b,c,d,e,f,g,dp for this digit

        // ensure all segments are off before enabling a new digit
        for (int s = 0; s < numSegPins; s++)
        {
            digitalWrite(segPins[s], LOW);
        }

        digitalWrite(digitPins[d], LOW); // enable cathode (sink)

        for (int s = 0; s < numSegPins; s++)
        {
            if (mask & (1 << s)) // test if this segment bit is set
            {
                digitalWrite(segPins[s], HIGH);
                delayMicroseconds(segmentPulseUs);
                digitalWrite(segPins[s], LOW);
            }
        }

        digitalWrite(digitPins[d], HIGH); // disable cathode

        // tiny blanking delay to reduce ghosting between digits
        delayMicroseconds(50);
    }
}

// Simple Wi-Fi connect helper
void connectWiFi()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    unsigned long start = millis();
    int wifistate = LOW;
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000)
    {
        // drive colon pins directly based on colonState
        digitalWrite(colonCathodes[0], wifistate);
        digitalWrite(colonPins[0], !wifistate);
        wifistate = !wifistate;
        delay(500);
    }
}
