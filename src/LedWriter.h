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

#ifndef LEDWRITER_H
#define LEDWRITER_H

#include <vector>
#include <stdint.h>
#include <thread>
#include "Effect.h"

#if !IS_EMBEDDED
    #include <chrono>
#endif

#define MAX_EFFECTS     1000
#define USE_TASKS       false

class GlobalSave
{
    public:
        bool enabled = false;
        uint32_t saveUID = 0, recallUID = 0;
        std::array<uint16_t, 3> saved = {{0, 0, 0}};
        void clear();
        void enable(uint32_t, uint32_t);
        void save(uint16_t, uint16_t, uint16_t);
        void save(std::array<uint16_t, 3>);
        std::array<uint16_t, 3> recall();
        bool recallInquiry(uint32_t);
        bool saveInquiry(uint32_t);
};


class LedWriter : public SimpleSerialBase {
    public:
        std::array<ColorChannel*, 3> channels;
        std::array<uint16_t*, 3> color;
        GlobalSave* globalSave;
        std::vector<Effect*> effects;
        Effect* effect = nullptr;
        bool inverted = false, verbose = false;
        double frequency, globalEffectDuration = 1e-6;
        uint8_t resolution;
        uint16_t absoluteMaximum, maximum, minimum = 0;
        uint32_t timeIndex = 0, now = 0, lastUID = 0, lastCompletion = 0;
        LedWriter(
                uint8_t redPin=15,
                uint8_t greenPin=13,
                uint8_t bluePin=12,
                uint8_t resolution=10,
                bool on=true
            );
        ~LedWriter();
        void setPolarityInversion(bool);
        void setScale(uint8_t redScale=1, uint8_t greenScale=1, uint8_t blueScale=1);
        void setOffset(uint16_t redOffset=0, uint16_t greenOffset=0, uint16_t blueOffset=0);
        void setMax(uint16_t, uint16_t, uint16_t);
        void setMax(uint16_t);
        void setMin(uint16_t, uint16_t, uint16_t);
        void setMin(uint16_t);
        std::array<uint16_t, 3> getMax();
        std::array<uint16_t, 3> getMin();
        void set(std::array<uint16_t, 3>, bool immediate=false);
        void set(uint16_t, uint16_t, uint16_t, bool immediate=false);
        Effect* createEffect(
                std::array<uint16_t, 3> target,
                double duration=0, bool recall=false, double relativeStart=0,
                double startVariation=0, double durationVariation=0,
                uint32_t effectUID=0, bool updateUID=false, int32_t loop=0
            );
        Effect* createEffectAbsolute(
                std::array<uint16_t, 3> target,
                double duration=0, bool recall=false, uint32_t absoluteStart=0,
                double startVariation=0, double durationVariation=0,
                uint32_t effectUID=0, bool updateUID=false, int32_t loop=0
            );
        void alignEffects();
        void write();
        void overwrite(std::array<uint16_t, 3>);
        void overwrite(uint16_t, uint16_t, uint16_t);
        void save(bool global=false);
        std::array<uint16_t, 3> recall(bool global=false, bool apply=true, bool immediate=true);
        std::array<uint16_t, 3> full(bool immediate=true);
        std::array<uint16_t, 3> clear(bool immediate=true);
        std::array<uint16_t, 3> primary(uint8_t, uint16_t value=0);
        std::array<uint16_t, 3> secondary(uint8_t, uint16_t value=0);
        std::array<uint16_t, 3> getCurrent();
        std::array<uint16_t, 3> getTarget();
        std::array<uint16_t, 3> getColorInversion(std::array<uint16_t, 3>);
        std::array<uint16_t, 3> getColorInversion();
        bool illuminated();
        bool isMax(bool absolute=false);
        bool isMin();
        bool isColor(std::array<uint16_t, 3>);
        bool isColor(uint16_t, uint16_t, uint16_t);
        void invert();
        void increment(std::array<int32_t, 3>, bool immediate=true);
        void increment(
                int32_t redValue=1, int32_t greenValue=1, int32_t blueValue=1,
                bool immediate=true
            );
        void updateTimers(uint32_t delta);
        bool updateClock(uint32_t* currentTime=nullptr, bool adjust=false);
        uint32_t effectsQueued();
        bool effectsActive();
        int looping();
        void skipEffect();
        void updateEffects(std::array<uint16_t, 3>);
        Effect* nextEffect();
        Effect* lastEffect();
        void cycleEffects();
        void clearEffects(bool cancel=true);
        void hold(double=0.5, double timeIndex=1, bool all=false);
        void holdLast(double=0.5, double timeIndex=1);
        void resume();
        bool bounceFlash(
                std::array<uint16_t, 3>, std::array<uint16_t, 3>,
                double fadeDuration=0, double holdDuration=0
            );
        bool bounce(double fadeDuration=0, double holdDuration=0);
        bool blink(double fadeDuration=0, double holdDuration=0);
        void rotate(double duration=0);
        void cycle(double duration=0);
        void test(double duration=1.5);
        void status();
        void run();
        friend void loop();
        void startTasks();
};

#endif
