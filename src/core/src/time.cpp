// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <memoria/v1/core/tools/time.hpp>

#include <unistd.h>
#include <string>
#include <iostream>
#include <sys/timeb.h>



namespace memoria {



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

}
