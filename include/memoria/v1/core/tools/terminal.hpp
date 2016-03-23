// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/types/types.hpp>

namespace memoria {
namespace tools     {

class TermImpl {
public:
    virtual const char* reds() const    = 0;
    virtual const char* rede() const    = 0;

    virtual const char* greens() const  = 0;
    virtual const char* greene() const  = 0;
};


class Term {

    static const TermImpl* term_;

public:
    static void init(int argc, const char** argv, const char** envp);

    static const char * red_s() {return term_->reds();}
    static const char * green_s() {return term_->greens();}

    static const char * red_e() {return term_->rede();}
    static const char * green_e() {return term_->greene();}
};


class MonochomeTerminal: public TermImpl {
    virtual const char* reds() const    {return "";}
    virtual const char* rede() const    {return "";}

    virtual const char* greens() const  {return "";}
    virtual const char* greene() const  {return "";}
};

}
}
