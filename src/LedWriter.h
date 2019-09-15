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

#include <stdint.h>
#include <thread>
#include <vector>
#include "Effect.h"

#if !IS_EMBEDDED
    #include <chrono>
#endif

#define MAX_EFFECTS     1000
#define USE_TASKS       false

template <unsigned int N=4>
class GlobalSave {
    public:
        bool enabled = false;
        uint32_t saveUID = 0, recallUID = 0;
        std::array<uint16_t, N> saved;
        void clear();
        void enable(uint32_t, uint32_t);
        void save(std::array<uint16_t, N>);
        std::array<uint16_t, N> recall();
        bool recallInquiry(uint32_t);
        bool saveInquiry(uint32_t);
};

template <unsigned int N=4>
class LedWriter : public SimpleSerialBase {
    public:
        std::array<ColorChannel*, N> channels;
        std::array<uint16_t*, N> color;
        GlobalSave<N>* globalSave;
        std::vector<Effect<N>*> effects;
        Effect<N>* effect = nullptr;
        bool inverted = false, verbose = false;
        double frequency, globalEffectDuration = 1e-6;
        uint8_t resolution;
        uint16_t absoluteMaximum, maximum, minimum = 0;
        uint32_t timeIndex = 0, now = 0, lastUID = 0, lastCompletion = 0;
        LedWriter(std::array<uint8_t, N>, uint8_t=10, bool=true);
        LedWriter(uint8_t=10, bool=true);
        void init(std::array<uint8_t, N>, uint8_t=10, bool=true);
        ~LedWriter();
        void setPolarityInversion(bool);
        void setScale(std::array<double, N>);
        void setOffset(std::array<int16_t, N>);
        void setMax(uint16_t);
        void setMax(std::array<uint16_t, N>);
        void setMin(uint16_t);
        void setMin(std::array<uint16_t, N>);
        std::array<uint16_t, N> getMax();
        std::array<uint16_t, N> getMin();
        void set(std::array<uint16_t, N>, bool immediate=false);
        Effect<N>* createEffect(
                std::array<uint16_t, N> target,
                double duration=0, bool recall=false, double relativeStart=0,
                double startVariation=0, double durationVariation=0,
                uint32_t effectUID=0, bool updateUID=false, int32_t loop=0
            );
        Effect<N>* createEffectAbsolute(
                std::array<uint16_t, N> target,
                double duration=0, bool recall=false, uint32_t absoluteStart=0,
                double startVariation=0, double durationVariation=0,
                uint32_t effectUID=0, bool updateUID=false, int32_t loop=0
            );
        void alignEffects();
        void write();
        void overwrite(std::array<uint16_t, N>);
        void save(bool global=false);
        std::array<uint16_t, N> recall(bool global=false, bool apply=true, bool immediate=true);
        std::array<uint16_t, N> full(bool immediate=true);
        std::array<uint16_t, N> clear(bool immediate=true);
        std::array<uint16_t, N> primary(uint8_t, uint16_t value=0);
        std::array<uint16_t, N> secondary(uint8_t, uint16_t value=0);
        std::array<uint16_t, N> getCurrent();
        std::array<uint16_t, N> getTarget();
        std::array<uint16_t, N> getColorInversion(std::array<uint16_t, N>);
        std::array<uint16_t, N> getColorInversion();
        bool illuminated();
        bool isMax(bool absolute=false);
        bool isMin();
        bool isColor(std::array<uint16_t, N>);
        void invert();
        void increment(std::array<int32_t, N>, bool=true);
        void updateTimers(uint32_t delta);
        bool updateClock(uint32_t* currentTime=nullptr, bool adjust=false);
        uint32_t effectsQueued();
        bool effectsActive();
        int looping();
        void skipEffect();
        void updateEffects(std::array<uint16_t, N>);
        Effect<N>* nextEffect();
        Effect<N>* lastEffect();
        void cycleEffects();
        void clearEffects(bool cancel=true);
        void hold(double=0.5, double timeIndex=1, bool all=false);
        void holdLast(double=0.5, double timeIndex=1);
        void resume();
        bool bounceFlash(
                std::array<uint16_t, N>, std::array<uint16_t, N>,
                double fadeDuration=0, double holdDuration=0
            );
        bool bounce(double fadeDuration=0, double holdDuration=0);
        bool blink(double fadeDuration=0, double holdDuration=0);
        void rotate(double duration=0, bool=false);
        void cycle(double duration=0, bool=false);
        void test(double duration=1.5);
        void status();
        void run();
        friend void loop();
        void startTasks();
};

#endif

template class GlobalSave<1>;
template class GlobalSave<2>;
template class GlobalSave<3>;
template class GlobalSave<4>;
template class GlobalSave<5>;

template class LedWriter<1>;
template class LedWriter<2>;
template class LedWriter<3>;
template class LedWriter<4>;
template class LedWriter<5>;
