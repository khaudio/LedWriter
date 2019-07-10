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

#include "LedWriter.h"

template <unsigned int N>
void GlobalSave<N>::clear()
{
    this->enabled = false;
    this->saveUID = 0;
    this->recallUID = 0;
}

template <unsigned int N>
void GlobalSave<N>::enable(uint32_t uidToSave, uint32_t uidToRecall)
{
    this->enabled = true;
    this->saveUID = uidToSave;
    this->recallUID = uidToRecall;
}

template <unsigned int N>
void GlobalSave<N>::save(std::array<uint16_t, N> values)
{
    for (int i = 0; i < N; ++i)
    {
        this->saved[i] = values[i];
    }
}

template <unsigned int N>
std::array<uint16_t, N> GlobalSave<N>::recall()
{
    return this->saved;
}

template <unsigned int N>
bool GlobalSave<N>::saveInquiry(uint32_t uid)
{
    return (this->enabled && (uid == this->saveUID));
}

template <unsigned int N>
bool GlobalSave<N>::recallInquiry(uint32_t uid)
{
    return (this->enabled && (uid == this->recallUID));
}

template <unsigned int N>
LedWriter<N>::LedWriter(
        std::array<uint8_t, N> pinArray, uint8_t resolution, bool on
    ) {
    init(pinArray, resolution, on);
}

template <unsigned int N>
void LedWriter<N>::init(std::array<uint8_t, N> pins, uint8_t resolution, bool on) {
    this->resolution = (resolution >= 1 && resolution <= 15 ? resolution : 8);
    uint16_t multiplied = std::pow(2, this->resolution);
    this->frequency = (80000000 / multiplied);
    this->absoluteMaximum = (multiplied - 1);
    this->maximum = this->absoluteMaximum;
    for (int i = 0; i < N; ++i) {
        this->channels[i] = new ColorChannel(
                pins[i], i, this->resolution, this->frequency
            );
        this->color[i] = this->channels[i]->color;
    }
    this->globalSave = new GlobalSave<N>;
    this->globalSave->save(getCurrent());
    setPolarityInversion(this->inverted);
    this->effects.reserve(MAX_EFFECTS);
    #ifdef IS_EMBEDDED
        this->timeIndex = micros();
    #endif
    on ? full(true) : clear(true);
    updateClock();
    #if USE_TASKS
        startTasks();
    #endif
}

template <unsigned int N>
LedWriter<N>::~LedWriter() {
    clearEffects();
    for (auto channel: this->channels) {
        delete channel;
        channel = nullptr;
    }
    delete this->globalSave;
    this->globalSave = nullptr;
}

template <unsigned int N>
void LedWriter<N>::setPolarityInversion(bool inverted) {
    this->inverted = inverted;
    for (auto channel: this->channels) {
        channel->inverted = this->inverted;
    }
    print("Set inversion");
}

template <unsigned int N>
void LedWriter<N>::setScale(std::array<double, N> values) {
    for (int i = 0; i < N; ++i) {
        this->channels[i]->scale = values[i];
    }
}

template <unsigned int N>
void LedWriter<N>::setOffset(std::array<int16_t, N> values) {
    for (int i = 0; i < N; ++i) {
        this->channels[i]->offset = values[i];
    }
}

template <unsigned int N>
void LedWriter<N>::setMax(uint16_t globalMax) {
    // Sets global max to both LedWriter instance as well as all color channels
    this->maximum = globalMax;
    for (int i = 0; i < N; ++i) {
        this->channels[i]->setMax(this->maximum);
    }
}

template <unsigned int N>
void LedWriter<N>::setMax(std::array<uint16_t, N> values) {
    // Sets specific soft maximum values for each color channel
    for (int i = 0; i < N; ++i) {
        this->channels[i]->setMax(values[i]);
    }
}

template <unsigned int N>
void LedWriter<N>::setMin(uint16_t globalMin) {
    // Sets global min to both LedWriter instance as well as all color channels
    this->minimum = globalMin;
    for (int i = 0; i < N; ++i) {
        this->channels[i]->setMin(this->minimum);
    }
}

template <unsigned int N>
void LedWriter<N>::setMin(std::array<uint16_t, N> values) {
    // Sets specific minimum values for each color channel
    for (int i = 0; i < N; ++i) {
        this->channels[i]->setMin(values[i]);
    }
}

template <unsigned int N>
std::array<uint16_t, N> LedWriter<N>::getMax() {
    std::array<uint16_t, N> softMaximum;
    for (int i = 0; i < N; ++i) {
        softMaximum[i] = this->channels[i]->maximum;
    }
    return softMaximum;
}

template <unsigned int N>
std::array<uint16_t, N> LedWriter<N>::getMin() {
    std::array<uint16_t, N> softMinimum;
    for (int i = 0; i < N; ++i) {
        softMinimum[i] = this->channels[i]->minimum;
    }
    return softMinimum;
}

template <unsigned int N>
void LedWriter<N>::set(std::array<uint16_t, N> values, bool immediate) {
    print("Setting values");
    if (immediate) {
        clearEffects();
        for (int i = 0; i < N; ++i) {
            this->channels[i]->set(values[i], immediate);
        }
    } else {
        createEffect(values);
    }
    print("Set values");
}

template <unsigned int N>
Effect<N>* LedWriter<N>::createEffect(
        std::array<uint16_t, N> target,
        double duration, bool recall, double relativeStart,
        double startVariation, double durationVariation,
        uint32_t effectUID, bool updateUID, int32_t loop
    ) {
    uint32_t absoluteStart;
    if ((relativeStart > 0) && (relativeStart <= 4294.967296)) {
        absoluteStart = this->now + static_cast<uint32_t>(relativeStart * 1000000);
    } else {
        absoluteStart = this->now;
    }
    return createEffectAbsolute(
            target, duration, recall, absoluteStart,
            startVariation, durationVariation,
            effectUID, updateUID, loop
        );
}

template <unsigned int N>
Effect<N>* LedWriter<N>::createEffectAbsolute(
        std::array<uint16_t, N> target,
        double duration, bool recall, uint32_t absoluteStart,
        double startVariation, double durationVariation,
        uint32_t effectUID, bool updateUID, int32_t loop
    ) {
    bool updated = false;
    Effect<N>* updatedEffect;
    duration = (duration ? duration : this->globalEffectDuration);
    if (updateUID) {
        for (auto effect: this->effects) {
            // Update specified effect if not already acitve
            if (effect->uid == effectUID) {
                effect->updateTimers(duration, absoluteStart);
                updated = true;
                updatedEffect = effect;
                print("Updated existing effect timer");
            }
        }
    }
    if (!updated) {
        #ifdef IS_EMBEDDED
            if ((this->effects.size() >= MAX_EFFECTS) || (ESP.getFreeHeap() < 32768)) {
                print("Insufficient memory for effect creation");
                return nullptr;
            }
        #endif
        this->effects.push_back(new Effect<N>(
                target, &this->channels, &this->now,
                duration, recall, absoluteStart,
                startVariation, durationVariation,
                effectUID, loop
            ));
        this->effects.back()->verbose = this->verbose;
        return this->effects.back();
    } else {
        return updatedEffect;
    }
    print("Created effect");
}

template <unsigned int N>
void LedWriter<N>::alignEffects() {
    if (!effectsQueued()) {
        return;
    }
    uint32_t last = this->effect->end;
    for (auto effect: this->effects) {
        if (effect != this->effect && effect->start < last) {
            int64_t delta = static_cast<int64_t>(effect->start) - static_cast<int64_t>(last);
            if (delta < 0) {
                effect->adjust(-delta);
                effect->defer();
            }
        }
        last = effect->end;
    }
}

template <unsigned int N>
void LedWriter<N>::write() {
    for (auto channel: this->channels) {
        channel->write();
    }
    print("Applied");
}

template <unsigned int N>
void LedWriter<N>::overwrite(std::array<uint16_t, N> values) {
    for (int i = 0; i < N; ++i) {
        this->channels[i]->overwrite(values[i]);
    }
    print("Overwritten");
}

template <unsigned int N>
void LedWriter<N>::increment(std::array<int32_t, N> values, bool immediate) {
    for (int i = 0; i < N; ++i) {
        this->channels[i]->increment(values[i], immediate);
    }
}

template <unsigned int N>
void LedWriter<N>::save(bool global) {
    print("Saving");
    if (global) {
        this->globalSave->save(getCurrent());
        this->globalSave->enabled = true;
    } else {
        for (auto channel: this->channels) {
            channel->save();
        }
    }
    print("Saved");
}

template <unsigned int N>
std::array<uint16_t, N> LedWriter<N>::recall(bool global, bool apply, bool immediate) {
    print("Recalling");
    std::array<uint16_t, N> recalled;
    if (global) {
        recalled = this->globalSave->recall();
        this->globalSave->clear();
    } else {
        for (int i = 0; i < N; ++i) {
            recalled[i] = this->channels[i]->recall();
        }
    }
    if (apply) {
        set(recalled, immediate);
    }
    print("Recalled");
    return recalled;
}

template <unsigned int N>
std::array<uint16_t, N> LedWriter<N>::full(bool immediate) {
    std::array<uint16_t, N> values;
    for (int i = 0; i < N; ++i) {
        values[i] = this->channels[i]->maximum;
    }
    set(values, immediate);
    return values;
}

template <unsigned int N>
std::array<uint16_t, N> LedWriter<N>::clear(bool immediate) {
    std::array<uint16_t, N> values;
    for (int i = 0; i < N; ++i) {
        values[i] = this->channels[i]->minimum;
    }
    set(values, immediate);
    return values;
}

template <unsigned int N>
std::array<uint16_t, N> LedWriter<N>::primary(uint8_t channel, uint16_t value) {
    value = (value ? value : this->channels[channel]->maximum);
    std::array<uint16_t, N> values;
    for (int i = 0; i < N; ++i) {
        values[i] = this->channels[i]->minimum;
    }
    if (N >= 3)
    {
        for (int i = 0; i < 3; ++i) {
            values[i] = ((i == channel) ? value : this->channels[i]->minimum);
        }
    }
    return values;
}

template <unsigned int N>
std::array<uint16_t, N> LedWriter<N>::secondary(uint8_t channel, uint16_t value) {
    value = (value ? value : this->channels[channel]->maximum);
    std::array<uint16_t, N> values;
    for (int i = 0; i < N; ++i) {
        values[i] = this->channels[i]->minimum;
    }
    if (N >= 3)
    {
        for (int i = 0, j = 1; i < 3; ++i, ++j) {
            j = (j < 3 ? j : 0);
            if (i == channel) {
                values[i] = value;
                values[j] = value;
            }
        }
    }
    return values;
}

template <unsigned int N>
std::array<uint16_t, N> LedWriter<N>::getCurrent() {
    std::array<uint16_t, N> values;
    for (int i = 0; i < N; ++i) {
        values[i] = *(this->color[i]);
    }
    return values;
}

template <unsigned int N>
std::array<uint16_t, N> LedWriter<N>::getTarget() {
    std::array<uint16_t, N> values;
    if (this->effect != nullptr) {
        for (int i = 0; i < N; ++i) {
            values[i] = this->effect->target[i];
        }
    } else {
        values = getCurrent();
    }
    return values;
}

template <unsigned int N>
std::array<uint16_t, N> LedWriter<N>::getColorInversion(std::array<uint16_t, N> values) {
    std::array<uint16_t, N> inverted;
    for (int i = 0; i < N; ++i) {
        inverted[i] = this->channels[i]->getColorInversion(values[i]);
    }
    return inverted;
}

template <unsigned int N>
std::array<uint16_t, N> LedWriter<N>::getColorInversion() {
    std::array<uint16_t, N> inverted;
    for (int i = 0; i < N; ++i) {
        inverted[i] = this->channels[i]->getColorInversion();
    }
    return inverted;
}

template <unsigned int N>
bool LedWriter<N>::illuminated() {
    // Whether any channel is on
    for (auto color: this->color) {
        if (*color) {
            return true;
        }
    }
    return false;
}

template <unsigned int N>
bool LedWriter<N>::isMax(bool absolute) {
    // Whether all channels are at maximum value
    for (auto channel: this->channels) {
        if (channel->value != (absolute ? channel->absoluteMaximum : channel->maximum)) {
            return false;
        }
    }
    return true;
}

template <unsigned int N>
bool LedWriter<N>::isMin() {
    // Whether all channels are at minimum value
    for (auto channel: this->channels) {
        if (channel->value != channel->minimum) {
            return false;
        }
    }
    return true;
}

template <unsigned int N>
bool LedWriter<N>::isColor(std::array<uint16_t, N> target) {
    for (int i = 0; i < N; ++i) {
        if (*(this->color[i]) != target[i]) {
            return false;
        }
    }
    return true;
}

template <unsigned int N>
void LedWriter<N>::invert() {
    print("Inverting current values");
    set(getColorInversion());
}

template <unsigned int N>
bool LedWriter<N>::updateClock(uint32_t* currentTime, bool adjust) {
    // Updates clock and returns whether a clock rollover has occured
    uint32_t before = this->now;
    if (currentTime != nullptr) {
        this->now = *currentTime;
        int64_t delta = (static_cast<int64_t>(this->now) - static_cast<int64_t>(before));
        if (adjust) {
            // if (ectsQueued()) {
            //     for (auto effect: this->effects) {
            //         effect->adjust(delta);
            //     }
            // }
            if (this->verbose) {
                Serial.printf("Adjusted time delta: %lld microseconds\n", delta);
            }
        }
        print("Clock synchronized");
    } else {
        #ifdef IS_EMBEDDED
            this->now += (micros() - this->timeIndex);
        #endif
    }

    #ifdef IS_EMBEDDED
        this->timeIndex = micros();
    #endif
    // Return whether a clock rollover has occured
    return ((this->now < before) && (!adjust));
}

template <unsigned int N>
uint32_t LedWriter<N>::effectsQueued() {
    return (this->effects.size());
}

template <unsigned int N>
bool LedWriter<N>::effectsActive() {
    return (effectsQueued() ? this->effect->active : false);
}

template <unsigned int N>
int LedWriter<N>::looping() {
    /* If any effects are self-looping, returns the greatest number
    of remaining loops. Returns -1 if found (indefinite). */
    int loop = 0;
    if (effectsQueued()) {
        for (auto effect: this->effects) {
            if (effect->loop == -1) {
                return -1;
            } else if (effect->loop > loop) {
                loop = effect->loop;
            }
        }
    }
    return loop;
}

template <unsigned int N>
void LedWriter<N>::skipEffect() {
    this->effect->cancel();
}

template <unsigned int N>
void LedWriter<N>::updateEffects(std::array<uint16_t, N> target) {
    for (auto effect: this->effects) {
        effect->target = target;
    }
    overwrite(target);
    skipEffect();
}

template <unsigned int N>
Effect<N>* LedWriter<N>::nextEffect() {
    return (effectsQueued() ? this->effects.front() : nullptr);
}

template <unsigned int N>
Effect<N>* LedWriter<N>::lastEffect() {
    return (effectsQueued() ? this->effects.back() : nullptr);
}

template <unsigned int N>
void LedWriter<N>::cycleEffects() {
    if (this->effect != nullptr) {
        if (this->effect->complete() && !this->effect->active) {
            this->lastUID = this->effect->uid;
            this->lastCompletion = this->now;
            if (this->globalSave->recallInquiry(this->effect->uid)) {
                recall(true, true, true);
            }
            if (this->effect != nullptr) {
                if (this->effect->loop != 0) {
                    if (this->verbose) {
                        Serial.printf("Looping effect UID %u", this->effect->uid);
                    }
                    this->effect->aborted = false;
                    this->effect->last = this->now;
                    this->effect->start = this->now;
                    this->effect->uid = this->effects.back()->uid + 1;
                    if (this->effect->loop > 0) {
                        this->effect->loop--;
                    }
                    this->effects.push_back(this->effect);
                } else {
                    delete this->effect;
                    this->effect = nullptr;
                }
            }
            this->effects.erase(this->effects.begin());
            this->effect = (effectsQueued() ? this->effects.front() : nullptr);
        } else {
            if (this->effect->run()) {
                if (this->globalSave->saveInquiry(this->effect->uid)) {
                    save(true);
                }
            }
        }
    } else if (effectsQueued() && (this->effect == nullptr)) {
        print("Setting effect");
        this->effect = this->effects.front();
    }
}

template <unsigned int N>
void LedWriter<N>::clearEffects(bool cancel) {
    print("Clearing effects");
    if (effectsQueued()) {
        int end = cancel ? 0 : 1;
        for (int i = this->effects.size() - 1; i >= end; --i) {
            if (this->verbose) {
                Serial.printf("\tClearing effect UID %d\n", this->effects[i]->uid);
            }
            if (this->effects[i] != nullptr) {
                delete this->effects[i];
            }
            this->effects.erase(this->effects.begin() + i);
        }
        if (cancel) {
            this->effect = nullptr;
        }
        if (this->effects.size() >= 10) {
            this->effects.shrink_to_fit();
        }
    }
    print("Effects cleared");
}

template <unsigned int N>
void LedWriter<N>::hold(double seconds, double timeIndex, bool all) {
    /* Holds active effect when fade is completed.
    Creates an effect with current color if none are queued. */
    if (effectsQueued()) {
        print("Setting hold on current effect");
        this->effects.front()->hold(seconds, timeIndex);
        if (all && (this->effects.size() > 1)) {
            for (int i = 1; i < this->effects.size(); i++) {
                this->effects[i]->hold(seconds, timeIndex);
            }
        }
    }
}

template <unsigned int N>
void LedWriter<N>::holdLast(double seconds, double timeIndex) {
    /* Holds active effect when fade is completed.
    Creates an effect with current color if none are queued. */
    if (effectsQueued()) {
        print("Setting hold on last queued effect");
        this->effects.back()->hold(seconds, timeIndex);
    }
}

template <unsigned int N>
void LedWriter<N>::resume() {
    // Resumes effect if one is active
    if (effectsQueued()) {
        this->effect->resume();
    }
}

template <unsigned int N>
bool LedWriter<N>::bounceFlash(
        std::array<uint16_t, N> first, std::array<uint16_t, N> second,
        double fadeDuration, double holdDuration
    ) {
    // Flashes between two colors when no effects are queued; non-blocking.
    if (!effectsQueued()) {
        std::array<uint16_t, N> target;
        if (isColor(first)) {
            target = second;
        } else {
            target = first;
        }
        createEffect(target, (fadeDuration <= 1e-6 ? 1e-6 : fadeDuration / 2));
        if (holdDuration) {
            holdLast(holdDuration, 1);
        }
        return true;
    }
    return false;
}

template <unsigned int N>
bool LedWriter<N>::bounce(double fadeDuration, double holdDuration) {
    // Inverts color when no effects are queued; non-blocking.
    if (!effectsQueued()) {
        if (!fadeDuration) {
            fadeDuration = this->globalEffectDuration;
        }
        createEffect(getColorInversion(), (fadeDuration <= 1e-6 ? 1e-6 : fadeDuration / 2));
        if (holdDuration) {
            holdLast(holdDuration, 1);
        }
        return true;
    }
    return false;
}

template <unsigned int N>
bool LedWriter<N>::blink(double fadeDuration, double holdDuration) {
    // Clears or recalls color when no effects are queued; non-blocking.
    if (!effectsQueued()) {
        std::array<uint16_t, N> target;
        if (!fadeDuration) {
            fadeDuration = this->globalEffectDuration;
        }
        if (!isMin()) {
            save(false);
            target = getMin();
        } else {
            target = recall(false, false, false);
        }
        createEffect(target, (fadeDuration <= 1e-6 ? 1e-6 : fadeDuration / 2));
        if (holdDuration) {
            holdLast(holdDuration, 1);
        }
        return true;
    }
    return false;
}

template <unsigned int N>
void LedWriter<N>::rotate(double duration) {
    // Cycles RYGCBM once; non-blocking.  Duration sets length of one full cycle.
    double divided = (duration ? duration : this->globalEffectDuration) / 6;
    for (int i = 0, j = 0; i < 3; i++, j += 2) {
        createEffect(primary(i), divided, false, 0, 0, 0, j, false, 0);
        createEffect(secondary(i), divided,  false, 0, 0, 0, j + 1, false, 0);
    }
}

template <unsigned int N>
void LedWriter<N>::cycle(double duration) {
    // Cycles RYGCBMW when no effects are queued; non-blocking.
    if (!effectsQueued()) {
        rotate(duration);
    }
}

template <unsigned int N>
void LedWriter<N>::test(double duration) {
    /*
        Immediately cycles RYGCBMW once and outputs a message
        over serial connection.
        Non-blocking, but does cancel any active or queued effects.
    */
    clearEffects();
    save(true);
    rotate(duration);
    Effect<N>* last = lastEffect();
    this->globalSave->enable(last->uid - 5, last->uid);
    print("Remote test successful");
}

template <unsigned int N>
void LedWriter<N>::status() {
    // Prints various member variables to serial console for inspection and comparison.
    if (this->verbose) {
        std::array<uint16_t, N> current = getCurrent();
        bool active = false;
        if (this->effect != nullptr) {
            active = (this->effect->active);
        }
        size_t queueSize = 0;
        for (auto effect: this->effects) {
            queueSize += sizeof(*effect);
        }
        Serial.printf("Current Time: %u\t", this->now);
        if (N >= 3)
        {
            Serial.printf("Current: R: %u G: %u B: %u", current[0], current[1], current[2]);
        }
        if (N >= 4) {
            Serial.printf(" W: %u\n", current[3]);
        }
        Serial.printf("\n");
        Serial.printf("Effects: Queued: %u\tMemory consumed: %u\t", effectsQueued(), queueSize);
        Serial.printf("Active: %s\t", (active ? "True" : "False"));
        Serial.printf("Global duration: %f\n", this->globalEffectDuration);
        Serial.printf("Last effect UID %u completed at %u\n", this->lastUID, this->lastCompletion);
        if (this->effect != nullptr) {
            int64_t delta = static_cast<int64_t>(this->effect->start) - static_cast<int64_t>(this->now);
            Serial.printf(
                    "Seconds until next effect (%u): %f (%lld us)\n",
                    this->effect->start,
                    static_cast<double>(delta / 1000000),
                    delta
                );
        }
        if (active) {
            this->effect->status();
        }
    }
}

template <unsigned int N>
void LedWriter<N>::run() {
    /*
        Required for processing fades; reference this from loop(); i.e.,
        any value set without explicitly specifying immediate application
        must be processed here as a time-based effect.
    */
    updateClock();
    cycleEffects();
}

template <int N=3>
void loop(void* parameter) {
    // Loop for threaded operation
    LedWriter<N>* instance = static_cast<LedWriter<N>*>(parameter);
    while (true) {
        instance->run();
        #if ESP32
            vTaskDelay(2);
        #else
            std::this_thread::yield();
        #endif
    }
}

template <unsigned int N>
void LedWriter<N>::startTasks() {
    print("Starting run task");
    #if ESP32
        xTaskCreatePinnedToCore(loop, "runner", 40000, this, 1, NULL, 1);
    #elif ESP8266
        xTaskCreate(loop, "runner", 40000, this, 1, NULL);
    #elif __AVR__
        return;
    #else
        std::thread(loop);
    #endif
    print("Run task started");
}
