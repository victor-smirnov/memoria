
// Copyright 2012 Victor Smirnov
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

#include <memoria/v1/memoria.hpp>

#include "../tests_inc.hpp"



namespace memoria {
namespace v1 {


class TemplateTestTask: public SPTestTask {

    typedef TemplateTestTask MyType;

    Int param_      = 1024;
    bool throw_ex_  = false;

    Int state_param_;

public:


    TemplateTestTask(): SPTestTask("Test")
    {
        MEMORIA_ADD_TEST_PARAM(param_)->setDescription("Basic test parameter");
        MEMORIA_ADD_TEST_PARAM(throw_ex_)->setDescription("Throw exception if true");


        MEMORIA_ADD_TEST_PARAM(state_param_)->state();

        MEMORIA_ADD_TEST_WITH_REPLAY(runFirst, replayFirst);
    }

    void runFirst()
    {
        out()<<"Precofigured Param: "<<param_<<endl;

        state_param_ = 12344321;

        param_ = 123456;

        if (throw_ex_)
            throw Exception(MEMORIA_SOURCE, "Fake Exception!");
    }


    void replayFirst()
    {
        out()<<"Param="<<param_<<endl;
    }

    virtual ~TemplateTestTask() noexcept {}

};


}}