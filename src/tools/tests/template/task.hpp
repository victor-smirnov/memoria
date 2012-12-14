
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_TEMPLATE_TASK_HPP_
#define MEMORIA_TESTS_TEMPLATE_TASK_HPP_

#include <memoria/memoria.hpp>

#include "../tests_inc.hpp"



namespace memoria {

using namespace memoria::vapi;


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

    void runFirst(ostream& out)
    {
        out<<"Precofigured Param: "<<param_<<endl;

        state_param_ = 12344321;

        param_ = 123456;

        if (throw_ex_)
            throw Exception(MEMORIA_SOURCE, "Fake Exception!");
    }


    void replayFirst(ostream& out)
    {
        out<<"Param="<<param_<<endl;
    }

    virtual ~TemplateTestTask() throw() {}

};


}


#endif
