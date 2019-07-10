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

#include "Effect.h"

Hold::Hold(double seconds, double startThreshold) : SimpleSerialBase() {
    if (!seconds) {
        return;
    }
    seconds = (seconds <= 4294967295 ? seconds : 4294967295); // Maximum 4294967295
    seconds = (seconds >= 0 ? seconds : 0); // Minimum 0
    this->remaining = seconds * 1000000; // Hold duration in microseconds
    this->duration = this->remaining; // Save total in case of reinitializion
    startThreshold = (startThreshold <= 1 ? startThreshold : 1); // Maximum 1
    startThreshold = (startThreshold >= 0 ? startThreshold : 0); // Minimum 0
    this->start = startThreshold;
}

Hold::~Hold() {
    print("\tDestroying hold");
}

void Hold::init(uint32_t totalSteps) {
    // Initializes hold; needs to be called again if calling Effect is looping
    if (!totalSteps) {
        return;
    }
    this->active = false, this->complete = false;
    // Reset remaining if hold is being reinitialized in Effect loop
    this->remaining = (
            this->remaining == this->duration
        ) ? this->remaining : this->duration;
    // Count down to trigger start
    this->threshold = totalSteps - (totalSteps * this->start);
    if (this->verbose) {
        Serial.printf(
                "\t\tCalculating effect hold for %f seconds at %f percent completion\n",
                static_cast<double>(this->remaining / 1000000),
                static_cast<float>(this->threshold * 100)
            );
    }
}

void Hold::status() {
    if (this->verbose) {
            Serial.printf(
                    "\t\tActive: %s, Threshold: %u, Remaining: %lld\n",
                    (
                            this->active ? "True" : "False"
                        ), this->threshold, this->remaining
                );
        }
}

bool Hold::step(uint32_t stepsRemaining) {
    // Return true if hold is active
    if (!this->active && (stepsRemaining <= this->threshold)) {
        this->active = true;
        if (this->verbose) {
            Serial.printf(
                    "\t\tActivating hold with threshold %u with %u effect steps remaining for %f seconds\n",
                    this->threshold, stepsRemaining, this->remaining * 1e-6
                );
        }
        this->last = micros();
    }
    if (this->active) {
        // Subtract elapsed microseconds from remaining hold time
        uint32_t now = micros();
        this->remaining -= (now - this->last);
        this->last = now;
        if (this->remaining <= 0){
            print("\t\tDeactivating hold");
            this->active = false;
            this->complete = true;
        }
    }
    return this->active;
}

template <unsigned int N>
Effect<N>::Effect(
        std::array<uint16_t, N> target,
        std::array<ColorChannel*, N>* channels,
        uint32_t* now,
        double duration,
        bool recall,
        uint32_t absoluteStart,
        double startVariation,
        double durationVariation,
        uint32_t uid,
        int32_t loop
    ) {
    this->channels = channels;
    this->recall = recall;
    for (int i = 0; i < N; ++i) {
        this->current[i] = (*this->channels)[i]->color;
        this->target[i] = (*this->channels)[i]->conform(target[i]);
        if (this->recall) {
            (*this->channels)[i]->save();
        }
    }
    setDuration(duration);
    this->now = now;
    this->last = *this->now;
    this->start = absoluteStart;
    this->end = (this->start + this->duration);
    this->uid = uid;
    this->loop = loop;
    varyStart(startVariation);
    varyDuration(durationVariation);
    defer();
}

template <unsigned int N>
Effect<N>::~Effect() {
    if (this->recall) {
        for (auto channel: *this->channels) {
            channel->recall(true);
        }
    }
    if (this->verbose) {
        Serial.printf(
                "\tDestroying Effect UID %u with end: %u at %u\n",
                this->uid, this->end, *this->now
            );
    }
    clearHolds();
    this->now = nullptr;
    this->channels = nullptr;
}

template <unsigned int N>
bool Effect<N>::complete() {
    if (this->aborted) {
        return true;
    } else if (!this->holds.empty()) {
        return false;
    }
    return targetReached();
}

template <unsigned int N>
bool Effect<N>::targetReached() {
    for (int i = 0; i < N; ++i) {
        if ((*this->channels)[i]->get() != this->target[i]) {
            return false;
        }
    }
    return true;
}

template <unsigned int N>
void Effect<N>::cancel() {
    this->aborted = true;
    this->active = true;
}

template <unsigned int N>
uint32_t Effect<N>::secondsToMicroseconds(double seconds) {
    if (seconds < 0) {
        seconds = 0; // Minimum zero
    } else if (seconds > 4294967295) {
        seconds = 4294967295; // Max ~71 minutes
    }
    return (seconds * 1000000);
}

template <unsigned int N>
void Effect<N>::setDuration(double seconds) {
    uint32_t microseconds = secondsToMicroseconds(seconds);
    // Minimum duration 1 microsecond
    this->duration = (microseconds ? microseconds : 1);
}

template <unsigned int N>
void Effect<N>::defer() {
    // Defer start if effect spans clock rollover
    if (this->deferOnRollover && (this->end < this->start)) {
        this->start = 1;
        this->end = (this->start + this->duration);
    }
}

template <unsigned int N>
void Effect<N>::adjust(int64_t delta) {
    // Adjust time points for clock synchronization
    if (!this->active) {
        this->start += delta;
        this->end += delta;
        this->last += delta;
    }
    defer();
}

template <unsigned int N>
void Effect<N>::updateTimers(double duration, uint32_t absoluteStart) {
    if (this->active) {
        return;
    }
    setDuration(duration);
    this->start = absoluteStart;
}

template <unsigned int N>
int16_t Effect<N>::vary(int32_t boundary, uint32_t& var) {
    if (!boundary) {
        return 0;
    }
    srand(micros());
    int16_t variance = rand();
    if (abs(variance) < boundary && (
            (var + variance) > *this->now
        )) {
        var += variance;
    }
    return variance;
}

template <unsigned int N>
void Effect<N>::varyStart(int32_t boundary) {
    this->end += vary(boundary, this->start);
}

template <unsigned int N>
void Effect<N>::varyDuration(int32_t boundary) {
    vary(boundary, this->duration);
    this->end = this->start + this->duration;
}

template <unsigned int N>
void Effect<N>::hold(double durationInSeconds, double timeIndex) {
    // Sets a hold in seconds that starts at timeIndex (0 - 1 effect completion)
    if (ESP.getFreeHeap() < 32768) {
        print("\tInsufficient memory for hold creation");
        return;
    }
    Hold* created = new Hold(durationInSeconds, timeIndex);
    created->verbose = this->verbose;
    this->holds.push_back(created);
}

template <unsigned int N>
void Effect<N>::resume() {
    // Cancel current hold
    if (!this->holds.empty()) {
        delete this->holds.front();
        this->holds.erase(this->holds.begin());
    }
}

template <unsigned int N>
void Effect<N>::clearHolds() {
    print("\tClearing holds");
    if (!this->holds.empty() || !this->secondaryHolds.empty()) {
        int holdsSize(this->holds.size()), secondaryHoldsSize(this->secondaryHolds.size());
        for (int i = 0; i < holdsSize; i++) {
            delete this->holds[i];
        }
        for (int i = 0; i < secondaryHoldsSize; i++) {
            delete this->secondaryHolds[i];
        }
        this->holds.clear();
        this->secondaryHolds.clear();
    }
    print("\tHolds cleared");
}

template <unsigned int N>
void Effect<N>::activate() {
    if (this->verbose) {
        Serial.printf(
                "\tActivating effect UID %u with start time %u at %u\n",
                this->uid, this->start, *this->now
            );
    }
    this->active = true;
    for (int i = 0; i < N; ++i) {
        (*this->channels)[i]->setTarget(this->target[i]);
    }
    if (!getSteps()) {
        return;
    }
    this->start = *this->now;
    this->end = *this->now + this->duration;
    // this->last = *this->now;
    this->last = micros();
    this->stepLength = (this->duration / this->stepsRemaining);
    this->stepLength = (this->stepLength ? this->stepLength : 1);
    if (!this->secondaryHolds.empty()) {
        for (auto saved: this->secondaryHolds) {
            saved->init(this->totalSteps);
            this->holds.push_back(saved);
        }
        this->secondaryHolds.clear();
    }
    if (this->verbose) {
        Serial.printf(
                "\tActivated effect UID %u with start time %u, end time %u at %u with %u steps\n",
                this->uid, this->start, this->end,
                *this->now, this->totalSteps
            );
        Serial.printf(
                "\tChannel step sizes: %f, %f, %f\n",
                (*this->channels)[0]->stepSize,
                (*this->channels)[1]->stepSize,
                (*this->channels)[2]->stepSize
            );
        status();
    }
}

template <unsigned int N>
uint32_t Effect<N>::getSteps() {
    for (auto channel: *this->channels) {
        channel->getDelta();
        if (channel->delta > this->stepsRemaining) {
            this->stepsRemaining = channel->delta;
        }
        this->totalSteps = this->stepsRemaining;
        channel->calculate(this->stepsRemaining);
    }
    this->aborted = (!this->stepsRemaining);
    return this->stepsRemaining;
}

template <unsigned int N>
bool Effect<N>::holding() {
    // Process holds and return whether a hold is active
    if (!this->holds.empty()) {
        Hold* current = this->holds.front();
        if (current->complete) {
            if (this->loop != 0) {
                this->secondaryHolds.push_back(this->holds.front());
            } else {
                delete current;
            }
            this->holds.erase(this->holds.begin());
            if (!this->holds.empty()) {
                current = this->holds.front();
                current->init(this->totalSteps);
            }
        } else {
            if (current->step(this->stepsRemaining)) {
                this->last = *this->now;
                return true;
            }
        }
    }
    return false;
}

template <unsigned int N>
void Effect<N>::step() {
    // Measures elapsed time and compensates
    uint32_t elapsed = micros() - this->last;
    if (elapsed && !holding() && (this->stepsRemaining > 0)) {
        int64_t iterations = elapsed / this->stepLength;
        if (iterations >= 1) {
            iterations = (
                    iterations <= this->stepsRemaining
                    ? iterations : this->stepsRemaining
                );
            for (int i = 0; i < iterations; i++) {
                this->stepsRemaining--;
                for (auto channel: *this->channels) {
                    channel->step();
                    if (!this->stepsRemaining && !targetReached()) {
                        while (channel->fading()) {
                            channel->step();
                        }
                    }
                }
                this->last = micros();
            }
        }
    }
}

template <unsigned int N>
bool Effect<N>::run() {
    /* Main loop for Effects;
    returns true upon activation (not while stepping). */
    if (this->active && complete()) {
        this->active = false;
        this->last = *this->now;
    } else if (this->active) {
        step();
    } else if (
            (*this->now > this->start)
            || ((*this->now < this->last) && (*this->now < this->start))
        ) {
        activate();
        return true;
    }
    return false;
}

template <unsigned int N>
void Effect<N>::status() {
    if (this->verbose) {
        Serial.printf("Effect status:\n");
        Serial.printf(
                "\tUID: %u\tComplete: %s\tActive: %s\tAborted: %s\n",
                this->uid,
                (complete() ? "True" : "False"),
                (this->active ? "True" : "False"),
                (this->aborted ? "True" : "False")
            );
        Serial.printf(
                "\tCurrent: %u, %u, %u Target: %u, %u, %u\n",
                *(this->current[0]), *(this->current[1]), *(this->current[2]),
                this->target[0], this->target[1], this->target[2]
            );
        Serial.printf(
                "\tStart: %u End: %u Duration: %u Now: %u\n",
                this->start, this->end, this->duration, *this->now
            );
        Serial.printf(
                "\tStep Length: %u Steps Remaining: %u\n",
                this->stepLength, this->stepsRemaining
            );
        double secondsRemaining = (
                this->stepsRemaining * this->stepLength
            ) / 1e6;
        Serial.printf("\tSeconds remaining: %f\n", secondsRemaining);
        Serial.printf(
                "\tHolds: %u\tLooping: %s\n",
                this->holds.size(),
                this->loop != 0 ? "True" : "False"
            );
        if (this->holds.size()) {
            this->holds.front()->status();
        }
    }
}
