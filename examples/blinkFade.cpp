#include "LedWriter.h"

std::array<uint8_t, 3> pins = {
        15, // Red output pin
        13, // Green output pin
        12, // Blue output pin
    };

LedWriter<3> writer(
        pins,
        10, // Resolution (1-15); 10-bit allows a range of 0-1023
        false // Whether to initialize output in constructor
    );

void setup()
{
    // Set target color value (R, G, B respectively)
    writer.set(
            1023, 416, 0, // Orange
            true // Set immediately (do not fade)
        );
    writer.save(); // Save current color value for later recall
}

void loop()
{
    // Blink runs when no effects are queued
    writer.blink(.5); // Blink over a period of .5 seconds
    writer.run(); // Required to process operations
}
