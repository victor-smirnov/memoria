// Copyright 2015 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include <memoria/core/tools/time.hpp>

#ifndef _MSC_VER
#include <unistd.h>
#endif

#include <string>
#include <iostream>
#include <sys/time.h>


namespace memoria {

int64_t getTimeInMillis()
{
    timespec tm;
    clock_gettime(CLOCK_REALTIME, &tm);
    return tm.tv_sec * 1000 + tm.tv_nsec / 1000000;
}


U8String getMillisPart(int64_t millis) {
    return millis < 100 ? U8String("0") + toString(millis) : toString(millis);
}

U8String getTwoDigitsPart(int64_t value) {
    return value < 10 ? U8String("0") + toString(value) : toString(value);
}

U8String FormatTime(int64_t millis)
{
    if (millis < 1000)
    {
        return U8String("0.") + getMillisPart(millis);
    }
    else if (millis < 60000)
    {
        int64_t seconds  = millis / 1000;
        millis = millis % 1000;

        return toString(seconds) + "." + getMillisPart(millis);
    }
    else if (millis < 60000 * 60)
    {
        int64_t minutes  = millis / 60000;

        millis -= minutes * 60000;

        int64_t seconds  = millis / 1000;
        millis = millis % 1000;

        return toString(minutes) + ":" + getTwoDigitsPart(seconds) + "." + getMillisPart(millis);
    }
    else if (millis < 60000 * 60 * 24)
    {
        int64_t hours    = millis / (60000 * 60);
        millis -= hours * 60000 * 60;

        int64_t minutes  = millis / 60000;

        millis -= minutes * 60000;

        int64_t seconds  = millis / 1000;
        millis = millis % 1000;

        return toString(hours) + ":" + getTwoDigitsPart(minutes)
                               + ":" + getTwoDigitsPart(seconds)
                               + "." + getMillisPart(millis);
    }
    else
    {
        int64_t days     = millis / (60000 * 60 * 24);
        millis -= days * 60000 * 60 * 24;

        int64_t hours    = millis / (60000 * 60);
        millis -= hours * 60000 * 60;

        int64_t minutes  = millis / 60000;
        millis -= minutes * 60000;

        int64_t seconds  = millis / 1000;
        millis = millis % 1000;

        return toString(days) + (days == 1? "day " : "days ")
                              + getTwoDigitsPart(hours)
                              + ":"
                              + getTwoDigitsPart(minutes)
                              + ":"
                              + getTwoDigitsPart(seconds)
                              + "."
                              + getMillisPart(millis);
    }
}

}
