#include "LedWriter.h"

LedWriter<3> writer(15, 13, 12); // RGB output pins and default resolution

void setup() {}

void loop()
{
    writer.blink(.5);
    writer.run();
}
