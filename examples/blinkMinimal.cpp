#include "LedWriter.h"

LedWriter<3> writer; // Default output pin numbers and resolution

void setup() {}

void loop()
{
    writer.blink(.5);
    writer.run();
}
