#include "LedWriter.h"

LedWriter<3> writer(
        15, // Red output pin
        13, // Green output pin
        12, // Blue output pin
        10, // Resolution (1-15); 10-bit allows a range of 0-1023
        true // Whether to initialize output in constructor
    );
double duration = .5, // Starting duration
dutyCycle = .5;

void setup()
{
    // Set default fade time in seconds
    writer.globalEffectDuration = 1e-6; // 1us for immediate transition
}

void loop()
{
    if (!writer.effectsQueued()) // If no effects are queued
    {
        duration = duration < .001 ? .5 : (duration * .5); // Halve duration to ramp
        double onTime = duration * dutyCycle; // Calculate high duration
        double offTime = duration - onTime; // Calculate low duration
        writer.set(writer.getMax()); // Set high value
        writer.holdLast(onTime); // Hold high value
        writer.set(writer.getMin()); // Set low value
        writer.holdLast(offTime); // Hold low value for remainder of duration
    }

    /* Newly set target and hold will not begin to increment
    until run() is reached. This eliminates problems caused
    by timing discrepancies in saving/recalling. */
    
    writer.run(); // Required to process operations
}
