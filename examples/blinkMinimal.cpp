#include "LedWriter.h"

LedWriter writer(15, 13, 12); // RGB output pins and default resolution

void setup() {}

void loop()
{
    writer.blink();
    writer.run();
}
