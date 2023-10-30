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


#pragma once

#include <memoria/core/types.hpp>

namespace memoria {
namespace tools {

bool IsATTY(int);

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

}}
