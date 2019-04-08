/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2019 K Hughes Production, LLC
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS products only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef COLORCHANNEL_H
#define COLORCHANNEL_H

#include <algorithm>
#include <stdint.h>
#include "SimpleSerialBase.h"

class ColorChannel : public SimpleSerialBase {
    public:
        bool verbose = false, inverted = false;
        double frequency, steps, stepSize;
        uint8_t pin, channel, resolution, scale = 1;
        uint16_t value, target, absoluteMaximum, maximum, minimum = 0, last = 0, offset = 0;
        uint16_t* color = &value;
        uint32_t delta, lastRounded;
        ColorChannel(uint8_t pin, uint8_t channel, uint8_t resolution=8, double frequency=0);
        ~ColorChannel();
        void write();
        void overwrite(uint16_t val, bool save=true);
        uint16_t conformAbsolute(uint16_t);
        uint16_t conform(uint16_t);
        uint16_t getColorInversion();
        uint16_t getColorInversion(uint16_t);
        void setMax(uint16_t value);
        void setMin(uint16_t value);
        void set(uint16_t, bool immediate=false);
        void setTarget(uint16_t);
        uint16_t get();
        void increment(int32_t value=1, bool immediate=false);
        void stepToward();
        void full();
        void clear();
        void invert(bool immediate=false);
        void save();
        void recall(bool immediate=false);
        bool fading();
        uint32_t getDelta();
        void calculate(uint32_t numSteps = 1);
        void step();
};

#endif
