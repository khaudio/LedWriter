#include "LedWriter.h"

LedWriter<3> writer(
        15, // Red output pin
        13, // Green output pin
        12, // Blue output pin
        10, // Resolution (1-15); 10-bit allows a range of 0-1023
        false // Whether to initialize output in constructor
    );

void setup() {}

void loop()
{
    writer.bounceFlash(
            std::array<uint16_t, 3>{1023, 416, 0}, // Orange
            std::array<uint16_t, 3>{536, 901, 360}, // Pale Yellow
            .25, // Fade over a period of 250ms
            .75 // Seconds to hold once each target is reached
        );
    writer.run(); // Required to process operations
}
