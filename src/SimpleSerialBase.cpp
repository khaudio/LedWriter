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

#include "SimpleSerialBase.h"
#include <cmath>

bool SimpleSerialBase::staticVerbose = false;

template <typename T>
void SimpleSerialBase::print(const T message, const char *endl)
{
    if (this->verbose)
    {
        sout << message << endl;
    }
}

template <typename T>
void SimpleSerialBase::prints(const T message, const char *endl)
{
    if (staticVerbose)
    {
        sout << message << endl;
    }
}

void SimpleSerialBase::print(const char* message, const char *endl)
{
    if (this->verbose)
    {
        sout << message << endl;
    }
}

void SimpleSerialBase::prints(const char* message, const char *endl)
{
    if (staticVerbose)
    {
        sout << message << endl;
    }
}

template <typename T>
void SimpleSerialBase::assigner(T& first, T& second)
{
    first = first ? first : second;
}

template <typename T>
T SimpleSerialBase::checker(T& first, T& second)
{
    return (first ? first : second);
}

template <typename T>
void SimpleSerialBase::conformUnsignedBoundaries(T& first, uint8_t bits)
{
    T maxPossible = (std::pow(2, bits) - 1);
    first = (first <= maxPossible ? first : maxPossible);
    first = (first >= 0 ? first : 0);
}
