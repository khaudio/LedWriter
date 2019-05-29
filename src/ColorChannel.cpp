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

#include "ColorChannel.h"

ColorChannel::ColorChannel(
        uint8_t pin,
        uint8_t channel,
        uint8_t resolution,
        double frequency
    ) {
    this->pin = pin;
    this->absoluteMaximum = (std::pow(2, resolution) - 1);
    this->maximum = this->absoluteMaximum;
    this->channel = channel;
    this->resolution = (resolution >= 1 && resolution <= 15 ? resolution : 8);
    double maxFrequency = (8e7 / std::pow(2, resolution));
    this->frequency = (
            frequency <= 0 || (frequency > maxFrequency)
        ) ? maxFrequency : frequency;
    #if ESP32 || ESP8266
        ledcAttachPin(this->pin, this->channel);
        ledcSetup(this->channel, this->frequency, this->resolution);
    #elif __AVR__
        pinMode(this->pin, OUTPUT);
    #endif
    set(this->maximum, true);
    this->target = this->value;
    save();
}

ColorChannel::~ColorChannel() {
    this->color = nullptr;
    #if ESP32 || ESP8266
        ledcDetachPin(this->pin);
    #endif
}

void ColorChannel::write() {
    #if ESP32 || ESP8266
        ledcWrite(this->channel, (
                this->inverted ? getColorInversion() : this->value
            ));
    #elif __AVR__
        analogWrite(this->channel, (
                this->inverted ? getColorInversion() : this->value
            ));
    #endif
}

void ColorChannel::overwrite(uint16_t val) {
    this->value = val;
    #if ESP32 || ESP8266
        ledcWrite(this->channel, val);
    #elif __AVR__
        analogWrite(this->channel, val);
    #endif
}

uint16_t ColorChannel::conformAbsolute(uint16_t val) {
    return val > this->absoluteMaximum ? this->absoluteMaximum : val;
}

uint16_t ColorChannel::conform(uint16_t val) {
    val = val > this->maximum ? this->maximum : val;
    val = val < this->minimum ? this->minimum : val;
    return val;
}

uint16_t ColorChannel::getColorInversion() {
    return (this->absoluteMaximum - this->value);
}

uint16_t ColorChannel::getColorInversion(uint16_t val) {
    return (this->absoluteMaximum - val);
}

void ColorChannel::setMax(uint16_t val) {
    this->maximum = conformAbsolute(val);
}

void ColorChannel::setMin(uint16_t val) {
    this->minimum = conformAbsolute(val);
}

void ColorChannel::set(uint16_t val, bool immediate) {
    this->value = conform((this->scale * val) + (this->offset));
    if (immediate) {
        write();
    }
}

void ColorChannel::setTarget(uint16_t val) {
    this->target = conform((this->scale * val) + (this->offset));
}

uint16_t ColorChannel::get() {
    return this->value;
}

void ColorChannel::increment(int32_t value, bool immediate) {
    set(static_cast<uint16_t>(this->value + value), immediate);
}

void ColorChannel::stepToward() {
    if (this->value > this->target) {
        increment(-1);
    } else if (this->value < this->target) {
        increment(1);
    }
}

void ColorChannel::full() {
    set(this->maximum);
}

void ColorChannel::clear() {
    set(this->minimum);
}

void ColorChannel::invert(bool immediate) {
    set(getColorInversion(), immediate);
}

void ColorChannel::save() {
    this->last = this->value;
}

uint16_t ColorChannel::recall(bool immediate) {
    this->value = this->last;
    this->target = this->value;
    if (immediate) {
        write();
    }
    return this->value;
}

bool ColorChannel::fading() {
    return (this->value != this->target);
}

uint32_t ColorChannel::getDelta() {
    this->delta = abs(
            static_cast<int32_t>(this->value)
            - static_cast<int32_t>(this->target)
        );
    return this->delta;
}

void ColorChannel::calculate(uint32_t numSteps) {
    if (numSteps) {
        this->stepSize = (
                static_cast<double>(this->delta)
                / static_cast<double>(numSteps)
            );
    } else {
        this->stepSize = 0;
    }
    this->lastRounded = 0;
    this->steps = 0;
}

void ColorChannel::step() {
    if (this->value != this->target) {
        if (this->steps < this->delta) {
            this->steps += this->stepSize;
        }
        uint32_t rounded = this->steps;
        if (rounded > this->lastRounded) {
            increment((this->target < this->value ? -1 : 1), true);
        }
        this->lastRounded = rounded;
    }
}