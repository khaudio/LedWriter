# LedWriter

Non-blocking effects framework for granular RGB color control of LEDs connected to ESP32 and ESP8266 devices.

LedWriter uses an array of color channels and increments these channel values using a system of queued Effects.  Delays during fades are also non-blocking, and are performed by adding Holds to Effects.


## Usage
The user is not required to manipulate nested objects to operate LedWriter.  Currently, all attributes and methods are public.  LedWriter gives the user as much control as possible, and relies on attr/method naming to be self-explanatory; e.g., `LedWriter::set()` and `LedWriter::getCurrent()`.

Instantiate an LedWriter object

```C++
LedWriter writer(
        15, // Red output pin
        13, // Green output pin
        12, // Blue output pin
        10, // Resolution (1-15); 10-bit allows a range of 0-1023
        false // Whether to initialize output in constructor
    );
```

Set the target color value

```C++
writer.set(1023, 416, 0); // Orange
```

And run it in the main loop

```C++
void loop()
{
    writer.run(); // Required to process operations
}
```

Blinking an LED is as simple as using `writer.blink()` in the main loop, which is roughly equivalent to 

```C++
std::array<uint16_t, 3> saved; // Array to save values to

void blink()
{
    if (!writer.effects.size()) // If no effects are queued
    {
        std::array<uint16_t, 3> target; // Create an array for target values
        bool isMin = true;
        for (int i = 0; i < 3; i++)
        {
            // Check if all channels are already at minimum
            if (writer.channels[i]->value != writer.channels[i]->minimum)
            {
                isMin = false;
                break;
            }
        }
        if (!isMin) // If not off
        {
            for (int i = 0; i < 3; i++)
            {
                // Save current values
                saved[i] = writer.channels[i]->value;
                // Set target to per-channel minimum
                target[i] = writer.channels[i]->minimum;
            }
        }
        else
        {
            target = saved; // Recall saved values
        }
        writer.set(target, false); // Create Effect to fade to target values
    }
}
```
