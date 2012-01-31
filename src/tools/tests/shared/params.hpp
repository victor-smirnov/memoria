
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

	Int 	step_;
	BigInt 	size_;
	Int 	btree_airity_;

	ReplayParams(StringRef name = "Replay"): TestReplayParams(name), step_(0), size_(0), btree_airity_(0)
	{
		Add("step", step_);
		Add("size", size_);
		Add("btreeAirity", btree_airity_);
	}
};

struct BTreeReplayParams: public ReplayParams {

	Int 	vector_idx_;
	String 	pairs_data_file_;
	String 	pairs_sorted_data_file_;

	BTreeReplayParams(StringRef name = "Replay"): ReplayParams(name), vector_idx_(0)
	{
		Add("vectorIdx", vector_idx_);
		Add("pairsDataFile", pairs_data_file_);
		Add("pairsSortedDataFile", pairs_sorted_data_file_);
	}

	virtual ~BTreeReplayParams() throw () {};

};


struct TestTaskParams: public TaskParametersSet {

	Int 	size_;
	Int 	btree_airity_;
	bool 	btree_random_airity_;


	TestTaskParams(StringRef name): TaskParametersSet(name)
	{
		Add("size", size_, 1024);
		Add("btreeAirity", btree_airity_, -1);
		Add("btreeRandomAirity", btree_random_airity_, true);
	}
};


}


#endif
