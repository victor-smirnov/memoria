
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_KV_MAP_PARAMS_HPP_
#define MEMORIA_TESTS_KV_MAP_PARAMS_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>



namespace memoria {


struct KVMapReplay: public TestReplayParams {

	Int step_;
	Int vector_idx_;
	Int size_;

	String pairs_data_file_;
	String pairs_sorted_data_file_;

	KVMapReplay(StringRef name = "Replay"): TestReplayParams(name), step_(0), vector_idx_(0), size_(0)
	{
		Add("step", step_);
		Add("vectorIdx", vector_idx_);
		Add("size", size_);
		Add("pairsDataFile", pairs_data_file_);
		Add("pairsSortedDataFile", pairs_sorted_data_file_);
	}

	virtual ~KVMapReplay() throw () {};

};


struct KVMapParams: public TaskParametersSet {

	Int size_;

	KVMapParams(): TaskParametersSet("KVMap")
	{
		Add("size", size_, 1024);
	}
};


}


#endif
