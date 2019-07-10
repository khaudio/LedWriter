#include "LedWriter.h"

// Endlessly cycles RYGCBM every 3 seconds

LedWriter<3> writer(
        15, // Red output pin
        13, // Green output pin
        12, // Blue output pin
        10, // Resolution (1-15); 10-bit allows a range of 0-1023
        false // Whether to initialize output in constructor
    );

void setup()
{
    Serial.begin(921600); // Enable serial output
    writer.verbose = true; // Enable verbose messages
}

void loop()
{
    writer.cycle(3); // 3 seconds total cycle time
    writer.run(); // Required to process operations
}
