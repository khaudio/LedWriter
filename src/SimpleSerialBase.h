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

#ifndef SIMPLESERIALBASE_H
#define SIMPLESERIALBASE_H

#if ESP32 || ESP8266 || __AVR__
    #include <Arduino.h>
#else
    #include <iostream>
#endif
#include <string>
#include <sstream>

#define sout SimpleSerialStream()

class SimpleSerialBase {
    public:
        bool verbose = false;
        static bool staticVerbose;
        void print(const char*, const char* endl="\n");
        static void prints(const char*, const char* endl="\n");
        template <typename T> void print(T, const char* endl="\n");
        template <typename T> static void prints(T, const char* endl="\n");
        template <typename T> static void assigner(T&, T&);
        template <typename T> static T checker(T&, T&);
        template <typename T> static void conformUnsignedBoundaries(T&, uint8_t bits=32);
};

class SimpleSerialStream {
    public:
        template <typename T>
        SimpleSerialStream &operator<<(const T &message) {
            #if ESP32 || ESP8266 || __AVR__
                Serial.print(message);
            #else
                std::cout << message;
            #endif
            return *this;
        }
        SimpleSerialStream &operator<<(std::ostream &(*stream)(std::ostream&)) {
            std::ostringstream sstream;
            sstream << stream;
            #if ESP32 || ESP8266 || __AVR__
                Serial.print(sstream.str().c_str());
            #else
                std::cout << sstream.str().c_str();
            #endif
            return *this;
        }
};

#endif
