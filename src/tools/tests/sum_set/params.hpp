
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_IDX_SET_PARAMS_HPP_
#define MEMORIA_TESTS_IDX_SET_PARAMS_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>



namespace memoria {


struct SumSetReplay: public TestReplayParams {

	Int step_;
	Int vector_idx_;
	Int size_;

	String pairs_data_file_;
	String pairs_sorted_data_file_;

	Int from_;
	Int to_;


	SumSetReplay(StringRef name = "SumSetReplay"): TestReplayParams(name), step_(0), vector_idx_(0), size_(0), from_(0), to_(0)
	{
		Add("step", step_);
		Add("vectorIdx", vector_idx_);
		Add("size", size_);
		Add("from", from_);
		Add("to", 	to_);

		Add("pairsDataFile", pairs_data_file_);
		Add("pairsSortedDataFile", pairs_sorted_data_file_);
	}

	virtual ~SumSetReplay() throw () {};



};


struct SumSetParams: public TaskParametersSet {

	Int size_;
	Int count_;

	SumSetParams(): TaskParametersSet("SumSet")
	{
		Add("size", size_, 1024);
		Add("count", count_, 1);
	}

};


}


#endif
