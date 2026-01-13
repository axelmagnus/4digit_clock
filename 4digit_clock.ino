// Simple offline 4-digit counter: 0000, 0001, ... 9999 then wraps.

// ESP32 Huzzah32 bare 4-digit 7-seg test
// Wiring (example):
//   Segments a,b,c,d,e,f,g,dp -> GPIOs below, no resistors
//   Digit cathodes D1..D4 -> each through 150–220 Ω to the GPIOs below
//   Cathodes sink to GND when LOW
// Use one segment at a time to keep current bounded by the cathode resistor.

const int segPins[8] = {21, 17, 16, 19, 18, 5, 4, 25}; // a,b,c,d,e,f,g,dp
const int digitPins[4] = {27, 33, 15, 32};             // D1..D4 cathodes

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

byte frame[4] = {0, 0, 0, 0};            // per-digit segment masks
const unsigned int segmentPulseUs = 600; // on-time per lit segment; tweak 300–800 for brightness/flicker
const int scanRepeats = 4;               // how many full multiplex passes per loop for brightness

void setup()
{
    // Serial.begin(115200);
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

    showNumber(0); // start at 0000
}

void loop()
{
    static unsigned long lastTick = 0;
    static int counter = 1234;

    // Simple 1 Hz counter to exercise digits
    unsigned long now = millis();
    if (now - lastTick >= 300)
    {
        lastTick = now;
        counter = (counter + 1) % 10000;
        showNumber(counter);
    }

    // Refresh the display a few times per loop for steadier brightness
    for (int i = 0; i < scanRepeats; i++)
    {
        scanOnceOneSegmentAtATime();
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
    // If you want a colon, repurpose dp on digit 1 or 2: frame[1] |= B10000000;
}

// Multiplex: for each digit, light only the segments it needs, one at a time.
void scanOnceOneSegmentAtATime()
{
    for (int d = 0; d < 4; d++)
    {
        byte mask = frame[d]; // single byte: bits 0-7 map to a,b,c,d,e,f,g,dp for this digit

        // ensure all segments are off before enabling a new digit
        for (int s = 0; s < 8; s++)
        {
            digitalWrite(segPins[s], LOW);
        }

        digitalWrite(digitPins[d], LOW); // enable cathode (sink)

        for (int s = 0; s < 8; s++)
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