// Copyright 2013 Victor Smirnov
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


#include <memoria/core/strings/strings.hpp>
#include <memoria/core/tools/terminal.hpp>

#ifndef _MSC_VER
#include <unistd.h>
#endif

#include <string>
#include <iostream>
#include <sys/time.h>


namespace memoria {
namespace tools {

const TermImpl* Term::term_;

class LinuxTerminal: public TermImpl {
    virtual const char* reds() const    {return "\033[1;31m";}
    virtual const char* rede() const    {return "\033[0m";}

    virtual const char* greens() const  {return "\033[1;32m";}
    virtual const char* greene() const  {return "\033[0m";}
};

MonochomeTerminal   mono_terminal_;
LinuxTerminal       linux_terminal_;



void Term::init(int argc, const char** argv, const char** envp)
{
    bool color_term = false;

    for (;*envp; envp++)
    {
        std::string entry(*envp);

        if (entry == "TERM=linux" || isStartsWith(entry, "TERM=xterm"))
        {
            color_term = true;
            break;
        }
    }

    if (color_term && IsATTY(1))
    {
        term_ = & linux_terminal_;
    }
    else {
        term_ = & mono_terminal_;
    }
}




int64_t getTimeInMillisT()
{
    timespec tm;
    clock_gettime(CLOCK_REALTIME, &tm);
    return tm.tv_sec * 1000 + tm.tv_nsec / 1000000;
}


U8String getMillisPartT(int64_t millis) {
    return millis < 100 ? U8String("0") + toString(millis) : toString(millis);
}

U8String getTwoDigitsPartT(int64_t value) {
    return value < 10 ? U8String("0") + toString(value) : toString(value);
}

U8String FormatTimeT(int64_t millis)
{
    if (millis < 1000)
    {
        return U8String("0.") + getMillisPartT(millis);
    }
    else if (millis < 60000)
    {
        int64_t seconds  = millis / 1000;
        millis = millis % 1000;

        return toString(seconds) +"." + getMillisPartT(millis);
    }
    else if (millis < 60000 * 60)
    {
        int64_t minutes  = millis / 60000;

        millis -= minutes * 60000;

        int64_t seconds  = millis / 1000;
        millis = millis % 1000;

        return toString(minutes) + ":" + getTwoDigitsPartT(seconds) + "." + getMillisPartT(millis);
    }
    else if (millis < 60000 * 60 * 24)
    {
        int64_t hours    = millis / (60000 * 60);
        millis -= hours * 60000 * 60;

        int64_t minutes  = millis / 60000;

        millis -= minutes * 60000;

        int64_t seconds  = millis / 1000;
        millis = millis % 1000;

        return toString(hours) + ":" + getTwoDigitsPartT(minutes)
                               + ":" + getTwoDigitsPartT(seconds)
                               + "." + getMillisPartT(millis);
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
                              + getTwoDigitsPartT(hours)
                              + ":"
                              + getTwoDigitsPartT(minutes)
                              + ":"
                              + getTwoDigitsPartT(seconds)
                              + "."
                              + getMillisPartT(millis);
    }
}




}}
