
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_SHARED_PARAMS_HPP_
#define MEMORIA_TESTS_SHARED_PARAMS_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>



namespace memoria {


struct TestTaskParams: public TaskParametersSet {

    Int     size_;
    Int     btree_branching_;
    bool    btree_random_branching_;


    TestTaskParams(StringRef name): TaskParametersSet(name),
            size_(200),
            btree_branching_(0),
            btree_random_branching_(true)
    {
        Add("size", size_);
        Add("btree_branching", btree_branching_);
        Add("btree_random_branching", btree_random_branching_);
    }
};


}


#endif
