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

void GlobalSave::clear()
{
    this->enabled = false;
    this->saveUID = 0;
    this->recallUID = 0;
}

void GlobalSave::enable(uint32_t uidToSave, uint32_t uidToRecall)
{
    this->enabled = true;
    this->saveUID = uidToSave;
    this->recallUID = uidToRecall;
}

void GlobalSave::save(uint16_t red, uint16_t green, uint16_t blue)
{
    this->saved = {red, green, blue};
}

void GlobalSave::save(std::array<uint16_t, 3> values)
{
    this->saved = values;
}

std::array<uint16_t, 3> GlobalSave::recall()
{
    return this->saved;
}

bool GlobalSave::saveInquiry(uint32_t uid)
{
    return (this->enabled && (uid == this->saveUID));
}


bool GlobalSave::recallInquiry(uint32_t uid)
{
    return (this->enabled && (uid == this->recallUID));
}

LedWriter::LedWriter(
        uint8_t redPin, uint8_t greenPin, uint8_t bluePin,
        uint8_t resolution, bool on
    ) {
    std::array<int, 3> pins = {redPin, greenPin, bluePin};
    this->resolution = (resolution >= 1 && resolution <= 15 ? resolution : 8);
    uint16_t multiplied = std::pow(2, this->resolution);
    this->frequency = (80000000 / multiplied);
    this->absoluteMaximum = (multiplied - 1);
    this->maximum = this->absoluteMaximum;
    for (int i = 0; i < 3; i++) {
        this->channels[i] = new ColorChannel(pins[i], i, this->resolution, this->frequency);
        this->color[i] = this->channels[i]->color;
    }
    this->globalSave = new GlobalSave;
    this->globalSave->save(getCurrent());
    setPolarityInversion(this->inverted);
    this->effects.reserve(MAX_EFFECTS);
    this->timeIndex = micros();
    on ? full(true) : clear(true);
    updateClock();
}

LedWriter::~LedWriter() {
    clearEffects();
    for (auto channel: this->channels) {
        delete channel;
        channel = nullptr;
    }
    delete this->globalSave;
    this->globalSave = nullptr;
}

void LedWriter::setPolarityInversion(bool inverted) {
    this->inverted = inverted;
    for (auto channel: this->channels) {
        channel->inverted = this->inverted;
    }
    print("Set inversion");
}

void LedWriter::setScale(uint8_t redScale, uint8_t greenScale, uint8_t blueScale) {
    std::array<uint8_t, 3> scaleValues = {redScale, greenScale, blueScale};
    for (int i = 0; i < 3; i++) {
        this->channels[i]->scale = scaleValues[i];
    }
}

void LedWriter::setOffset(uint16_t redOffset, uint16_t greenOffset, uint16_t blueOffset) {
    std::array<uint16_t, 3> offsetValues = {redOffset, greenOffset, blueOffset};
    for (int i = 0; i < 3; i++) {
        this->channels[i]->offset = offsetValues[i];
    }
}

void LedWriter::setMax(uint16_t globalMax) {
    // Sets global max to both LedWriter instance as well as all color channels
    this->maximum = globalMax;
    setMax(globalMax, globalMax, globalMax);
}

void LedWriter::setMax(uint16_t redMax, uint16_t greenMax, uint16_t blueMax) {
    // Sets specific soft maximum values for each color channel
    std::array<uint16_t, 3> maxValues = {redMax, greenMax, blueMax};
    for (int i = 0; i < 3; i++) {
        this->channels[i]->setMax(maxValues[i]);
    }
}

void LedWriter::setMin(uint16_t globalMin) {
    // Sets global min to both LedWriter instance as well as all color channels
    this->minimum = globalMin;
    setMin(globalMin, globalMin, globalMin);
}

void LedWriter::setMin(uint16_t redMin, uint16_t greenMin, uint16_t blueMin) {
    // Sets specific minimum values for each color channel
    std::array<uint16_t, 3> minValues = {redMin, greenMin, blueMin};
    for (int i = 0; i < 3; i++) {
        this->channels[i]->setMin(minValues[i]);
    }
}

void LedWriter::set(std::array<uint16_t, 3> values, bool immediate) {
    if (immediate) {
        clearEffects();
        for (int i = 0; i < 3; i++) {
            this->channels[i]->set(values[i], immediate);
        }
    } else {
        createEffect(values);
    }
    print("Set values");
}

void LedWriter::set(
        uint16_t redValue, uint16_t greenValue, uint16_t blueValue,
        bool immediate
    ) {
    set(std::array<uint16_t, 3>{redValue, greenValue, blueValue}, immediate);
}

Effect* LedWriter::createEffect(
        std::array<uint16_t, 3> target,
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

Effect* LedWriter::createEffectAbsolute(
        std::array<uint16_t, 3> target,
        double duration, bool recall, uint32_t absoluteStart,
        double startVariation, double durationVariation,
        uint32_t effectUID, bool updateUID, int32_t loop
    ) {
    bool updated = false;
    Effect* updatedEffect;
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
        if ((this->effects.size() >= MAX_EFFECTS) || (ESP.getFreeHeap() < 32768)) {
            print("Insufficient memory for effect creation");
            return nullptr;
        }
        this->effects.push_back(new Effect(
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

void LedWriter::alignEffects() {
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

void LedWriter::write() {
    for (auto channel: this->channels) {
        channel->write();
    }
    print("Applied");
}

void LedWriter::overwrite(std::array<uint16_t, 3> values, bool save) {
    for (int i = 0; i < 3; i++) {
        this->channels[i]->overwrite(values[i], save);
    }
    print("Overwritten");
}

void LedWriter::overwrite(uint16_t redValue, uint16_t greenValue, uint16_t blueValue, bool save) {
    std::array<uint16_t, 3> values = {redValue, greenValue, blueValue};
    overwrite(values, save);
}

void LedWriter::increment(std::array<int32_t, 3> values, bool immediate) {
    for (int i = 0; i < 3; i++) {
        this->channels[i]->increment(values[i], immediate);
    }
}

void LedWriter::increment(
        int32_t redValue, int32_t greenValue, int32_t blueValue,
        bool immediate
    ) {
    increment(std::array<int32_t, 3> {redValue, greenValue, blueValue}, immediate);
}

void LedWriter::save(bool global) {
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

void LedWriter::recall(bool global, bool immediate) {
    if (global) {
        set(this->globalSave->recall(), immediate);
        this->globalSave->clear();
    } else {
        for (auto channel: this->channels) {
            channel->recall(immediate);
        }
    }
    print("Recalled");
}

std::array<uint16_t, 3> LedWriter::full(bool immediate) {
    std::array<uint16_t, 3> values = {
            this->channels[0]->maximum,
            this->channels[1]->maximum,
            this->channels[2]->maximum
        };
    set(values, immediate);
    return values;
}

std::array<uint16_t, 3> LedWriter::clear(bool immediate) {
    std::array<uint16_t, 3> values = {
            this->channels[0]->minimum,
            this->channels[1]->minimum,
            this->channels[2]->minimum
        };
    set(values, immediate);
    return values;
}

std::array<uint16_t, 3> LedWriter::primary(uint8_t channel, uint16_t value) {
    value = (value ? value : this->channels[channel]->maximum);
    std::array<uint16_t, 3> values;
    for (int i = 0; i < 3; i++) {
        values[i] = ((i == channel) ? value : this->channels[i]->minimum);
    }
    return values;
}

std::array<uint16_t, 3> LedWriter::secondary(uint8_t channel, uint16_t value) {
    value = (value ? value : this->channels[channel]->maximum);
    std::array<uint16_t, 3> values = {
            this->channels[0]->minimum,
            this->channels[1]->minimum,
            this->channels[2]->minimum
        };
    for (int i = 0, j = 1; i < 3; i++, j++) {
        j = (j < 3 ? j : 0);
        if (i == channel) {
            values[i] = value;
            values[j] = value;
        }
    }
    return values;
}

std::array<uint16_t, 3> LedWriter::getCurrent() {
    return std::array<uint16_t, 3>{
            *(this->color[0]),
            *(this->color[1]),
            *(this->color[2])
        };
}

std::array<uint16_t, 3> LedWriter::getTarget() {
    std::array<uint16_t, 3> values;
    if (this->effect != nullptr) {
        values = std::array<uint16_t, 3>{
                this->effect->target[0],
                this->effect->target[1],
                this->effect->target[2]
            };
    } else {
        values = getCurrent();
    }
    return values;
}

std::array<uint16_t, 3> LedWriter::getColorInversion(std::array<uint16_t, 3> values) {
    std::array<uint16_t, 3> inverted;
    for (int i = 0; i < 3; i++) {
        inverted[i] = this->channels[i]->getColorInversion(values[i]);
    }
    return inverted;
}

std::array<uint16_t, 3> LedWriter::getColorInversion() {
    std::array<uint16_t, 3> inverted;
    for (int i = 0; i < 3; i++) {
        inverted[i] = this->channels[i]->getColorInversion();
    }
    return inverted;
}

void LedWriter::invert() {
    print("Inverting current values");
    set(getColorInversion());
}

bool LedWriter::illuminated() {
    for (auto color: this->color) {
        if (*color) {
            return true;
        }
    }
    return false;
}

bool LedWriter::updateClock(uint32_t* currentTime, bool adjust) {
    // Updates clock and returns whether a clock rollover has occured
    uint32_t before = this->now;
    if (currentTime != nullptr) {
        this->now = *currentTime;
        int64_t delta = (static_cast<int64_t>(this->now) - static_cast<int64_t>(before));
        if (adjust) {
            // if (effectsQueued()) {
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
        this->now += (micros() - this->timeIndex);
    }
    this->timeIndex = micros();
    // Return whether a clock rollover has occured
    return ((this->now < before) && (!adjust));
}

uint32_t LedWriter::effectsQueued() {
    return (this->effects.size());
}

bool LedWriter::effectsActive() {
    return (effectsQueued() ? this->effect->active : false);
}

int LedWriter::looping() {
    /*
        If any effects are self-looping, returns the greatest number
        of remaining loops. Returns -1 if found (indefinite).
    */
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

void LedWriter::skipEffect() {
    this->effect->cancel();
}

void LedWriter::updateEffects(std::array<uint16_t, 3> target) {
    for (auto effect: this->effects) {
        effect->target = target;
    }
    overwrite(target, true);
    skipEffect();
}

Effect* LedWriter::nextEffect() {
    return (effectsQueued() ? this->effects.front() : nullptr);
}

Effect* LedWriter::lastEffect() {
    return (effectsQueued() ? this->effects.back() : nullptr);
}

void LedWriter::cycleEffects() {
    if (this->effect != nullptr) {
        if (this->effect->complete() && !this->effect->active) {
            this->lastUID = this->effect->uid;
            this->lastCompletion = this->now;
            if (this->globalSave->recallInquiry(this->effect->uid)) {
                recall(true, true);
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
                }
            }
            this->effects.erase(this->effects.begin());
            if (this->effects.size() >= 10) {
                this->effects.shrink_to_fit();
            }
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

void LedWriter::clearEffects(bool cancel) {
    print("Clearing effects");
    if (effectsQueued()) {
        if (cancel) {
            this->effect = nullptr;
        }
        int end = cancel ? 0 : 1;
        for (int i = this->effects.size() - 1; i >= end; --i) {
            if (this->verbose) {
                Serial.printf("Deleting effect UID %d\n", this->effects[i]->uid);
            }
            delete this->effects[i];
            this->effects.erase(this->effects.begin() + i);
        }
        if (this->effects.size() >= 10) {
            this->effects.shrink_to_fit();
        }
    }
    print("Effects cleared");
}

void LedWriter::hold(double seconds, double timeIndex, bool all) {
    /*
        Holds active effect when fade is completed.
        Creates an effect with current color if none are queued.
    */
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

void LedWriter::resume() {
    // Resumes effect if one is active
    if (effectsQueued()) {
        this->effect->resume();
    }
}

void LedWriter::bounce() {
    // Inverts colors when no effects are queued; non-blocking.
    if (!effectsQueued()) {
        set(getColorInversion());
    }
}

void LedWriter::rotate(double duration) {
    // Cycles RYGCBM once; non-blocking.  Duration sets length of one full cycle.
    double divided = (duration ? duration : this->globalEffectDuration) / 6;
    for (int i = 0; i < 6; i += 2) {
        createEffect(primary(i), divided, false, 0, 0, 0, i, false, 0);
        createEffect(secondary(i), divided,  false, 0, 0, 0, i + 1, false, 0);
    }
}

void LedWriter::cycle(double duration) {
    // Cycles RYGCBMW when no effects are queued; non-blocking.
    if (!effectsQueued()) {
        rotate(duration);
    }
}

void LedWriter::test(double duration) {
    /*
        Immediately cycles RYGCBMW once and outputs a message
        over serial connection.
        Non-blocking, but does cancel any active or queued effects.
    */
    clearEffects();
    save(true);
    rotate(duration);
    Effect* last = lastEffect();
    this->globalSave->enable(last->uid - 5, last->uid);
    print("Remote test successful");
}

void LedWriter::status() {
    // Prints various member variables to serial console for inspection and comparison.
    if (this->verbose) {
        std::array<uint16_t, 3> current = getCurrent();
        bool active = false;
        if (this->effect != nullptr) {
            active = (this->effect->active);
        }
        size_t queueSize = 0;
        for (auto effect: this->effects) {
            queueSize += sizeof(*effect);
        }
        Serial.printf("Current Time: %u\t", this->now);
        Serial.printf("Current: R: %u G: %u B: %u\n", current[0], current[1], current[2]);
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

void LedWriter::run() {
    /*
        Required for processing fades; reference this from loop(); i.e.,
        any value set without explicitly specifying immediate application
        must be processed here as a time-based effect.
    */
    updateClock();
    cycleEffects();
}
