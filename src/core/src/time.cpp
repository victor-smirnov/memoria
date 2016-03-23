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


#include <memoria/v1/core/tools/time.hpp>

#include <unistd.h>
#include <string>
#include <iostream>
#include <sys/timeb.h>



namespace memoria {
namespace v1 {



BigInt getTimeInMillis()
{
    struct timeb tm;
    ftime(&tm);
    return tm.time * 1000 + tm.millitm;
}


String getMillisPart(BigInt millis) {
    return millis < 100 ? "0"+toString(millis) : toString(millis);
}

String getTwoDigitsPart(BigInt value) {
    return value < 10 ? "0"+toString(value) : toString(value);
}

String FormatTime(BigInt millis)
{
    if (millis < 1000)
    {
        return "0." + getMillisPart(millis);
    }
    else if (millis < 60000)
    {
        BigInt seconds  = millis / 1000;
        millis = millis % 1000;

        return toString(seconds) +"." + getMillisPart(millis);
    }
    else if (millis < 60000 * 60)
    {
        BigInt minutes  = millis / 60000;

        millis -= minutes * 60000;

        BigInt seconds  = millis / 1000;
        millis = millis % 1000;

        return toString(minutes) +":" + getTwoDigitsPart(seconds) +"." + getMillisPart(millis);
    }
    else if (millis < 60000 * 60 * 24)
    {
        BigInt hours    = millis / (60000 * 60);
        millis -= hours * 60000 * 60;

        BigInt minutes  = millis / 60000;

        millis -= minutes * 60000;

        BigInt seconds  = millis / 1000;
        millis = millis % 1000;

        return toString(hours) + ":" + getTwoDigitsPart(minutes)
                               + ":" + getTwoDigitsPart(seconds)
                               + "." + getMillisPart(millis);
    }
    else
    {
        BigInt days     = millis / (60000 * 60 * 24);
        millis -= days * 60000 * 60 * 24;

        BigInt hours    = millis / (60000 * 60);
        millis -= hours * 60000 * 60;

        BigInt minutes  = millis / 60000;
        millis -= minutes * 60000;

        BigInt seconds  = millis / 1000;
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

}}