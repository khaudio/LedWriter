#include "LedWriter.h"

LedWriter<3> writer(
        15, // Red output pin
        13, // Green output pin
        12, // Blue output pin
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
    writer.blink(
            1e-6, // 1us fade for immediate transition
            .5 // How many seconds to hold values for
        );

    /* Newly set target and hold will not process until run() is reached.
    This eliminates problems caused by timing discrepancies
    in saving/recalling. */
    
    writer.run(); // Required to process operations
}
