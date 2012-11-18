
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


struct ReplayParams: public TestReplayParams {

    Int     step_;
    BigInt  size_;
    BigInt  ctr_name_;

    ReplayParams(StringRef name = "Replay"): TestReplayParams(name), step_(0), size_(0), ctr_name_(1)
    {
        Add("step", step_);
        Add("size", size_);
        Add("ctr_name", ctr_name_);
    }
};

struct BTreeReplayParams: public ReplayParams {

    Int     vector_idx_;
    String  pairs_data_file_;
    String  pairs_sorted_data_file_;
    Int     iteration_;


    BTreeReplayParams(StringRef name = "Replay"): ReplayParams(name), vector_idx_(0), iteration_(0)
    {
        Add("vectorIdx", vector_idx_);
        Add("pairsDataFile", pairs_data_file_);
        Add("pairsSortedDataFile", pairs_sorted_data_file_);
        Add("iteration", iteration_);
    }

    virtual ~BTreeReplayParams() throw () {};

};


struct TestTaskParams: public TaskParametersset {

    Int     size_;
    Int     btree_branching_;
    bool    btree_random_branching_;


    TestTaskParams(StringRef name): TaskParametersset(name),
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
