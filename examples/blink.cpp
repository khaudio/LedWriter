#include "LedWriter.h"

LedWriter writer(
        15, // Red output pin
        13, // Green output pin
        12, // Blue output pin
        10, // Resolution (1-15)
        true // Turn all channels on in constructor
    );

void setup()
{
    // Create an effect
    writer.createEffect(
        // Target color for the effect to reach
        std::array<uint16_t, 3>{238, 751, 851},
        0.5, // Duration in seconds for the fade to last
        false, // Whether to recall previous value after effect completes
        0.0, // Start immediately (0 seconds from now)
        0.0, // Boundary in seconds to randomize start time
        0.0, // Boundary in seconds to randomize duration
        0, // Unique effect ID,
        false, // Whether to update staged effect with same UID
        0 // How many times to loop the effect internally before destroying it
    );
}

void loop()
{
    /*
    If no effects are staged, invert
    the channel values and fade using defaults
    */
    writer.bounce();

    writer.run(); // Required loop for operation
}