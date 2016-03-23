// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <memoria/v1/core/tools/strings/string.hpp>
#include <memoria/v1/core/tools/terminal.hpp>

#include <unistd.h>
#include <string>
#include <iostream>
#include <sys/timeb.h>


namespace memoria {
namespace v1 {
namespace tools     {

using namespace std;

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
        string entry(*envp);

        if (entry == "TERM=linux" || isStartsWith(entry, "TERM=xterm"))
        {
            color_term = true;
            break;
        }
    }

    if (color_term && isatty(1))
    {
        term_ = & linux_terminal_;
    }
    else {
        term_ = & mono_terminal_;
    }
}




BigInt getTimeInMillisT()
{
    struct timeb tm;
    ftime(&tm);
    return tm.time * 1000 + tm.millitm;
}


String getMillisPartT(BigInt millis) {
    return millis < 100 ? "0"+toString(millis) : toString(millis);
}

String getTwoDigitsPartT(BigInt value) {
    return value < 10 ? "0"+toString(value) : toString(value);
}

String FormatTimeT(BigInt millis)
{
    if (millis < 1000)
    {
        return "0." + getMillisPartT(millis);
    }
    else if (millis < 60000)
    {
        BigInt seconds  = millis / 1000;
        millis = millis % 1000;

        return toString(seconds) +"." + getMillisPartT(millis);
    }
    else if (millis < 60000 * 60)
    {
        BigInt minutes  = millis / 60000;

        millis -= minutes * 60000;

        BigInt seconds  = millis / 1000;
        millis = millis % 1000;

        return toString(minutes) +":" + getTwoDigitsPartT(seconds) +"." + getMillisPartT(millis);
    }
    else if (millis < 60000 * 60 * 24)
    {
        BigInt hours    = millis / (60000 * 60);
        millis -= hours * 60000 * 60;

        BigInt minutes  = millis / 60000;

        millis -= minutes * 60000;

        BigInt seconds  = millis / 1000;
        millis = millis % 1000;

        return toString(hours) + ":" + getTwoDigitsPartT(minutes)
                               + ":" + getTwoDigitsPartT(seconds)
                               + "." + getMillisPartT(millis);
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
                              + getTwoDigitsPartT(hours)
                              + ":"
                              + getTwoDigitsPartT(minutes)
                              + ":"
                              + getTwoDigitsPartT(seconds)
                              + "."
                              + getMillisPartT(millis);
    }
}




}
}}