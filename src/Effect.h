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

#ifndef EFFECT_H
#define EFFECT_H

#include <array>
#include "ColorChannel.h"

class Hold : public SimpleSerialBase {
    public:
        bool active = false, complete = false, verbose = false;
        double start = 1; // 0 - 1 amount of effect completed to trigger hold
        uint32_t last, threshold = 0; // Descending effect step count to activate
        int64_t duration, remaining = 0;
        Hold(double, double startThreshold=1);
        ~Hold();
        void init(uint32_t);
        void status();
        bool step(uint32_t);
};

class Effect : public SimpleSerialBase {
    protected:
        int16_t vary(int32_t, uint32_t&);
    public:
        bool
            verbose = false, active = false, recall = false,
            deferOnRollover = true, aborted = false;
        uint32_t
            *now, last, start, end, duration, uid,
            stepLength = 1, stepsRemaining = 0, totalSteps = 0;
        int32_t loop = 0;
        std::array<uint16_t*, 3> current, globalLast;
        std::array<uint16_t, 3> target;
        std::array<ColorChannel*, 3>* channels;
        std::vector<Hold*> holds, secondaryHolds;
        Effect(
                std::array<uint16_t, 3> target,
                std::array<ColorChannel*, 3>* channels,
                uint32_t* now,
                double duration=0,
                bool recall=false,
                uint32_t absoluteStart=0,
                double startVariation=0,
                double durationVariation=0,
                uint32_t uid=0,
                int32_t loop=0
            );
        ~Effect();
        bool complete();
        bool targetReached();
        void cancel();
        static uint32_t secondsToMicroseconds(double);
        void setDuration(double seconds=1e-6);
        void defer();
        void adjust(int64_t delta);
        void updateTimers(double duration=0, uint32_t absoluteStart=0);
        void varyStart(int32_t boundary=0);
        void varyDuration(int32_t boundary=0);
        void activate();
        void hold(double, double timeIndex=1);
        void resume();
        void clearHolds();
        uint32_t getSteps();
        bool holding();
        void step();
        bool run();
        void status();
};

#endif