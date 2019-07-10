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
    // Create an Effect manually
    writer.createEffect(
            // Target color for the effect to reach
            std::array<uint16_t, 3>{238, 751, 851},
            8, // Duration in seconds for the fade to last
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
    // Effect will run once before destruction
    writer.run(); // Required to process operations
}
