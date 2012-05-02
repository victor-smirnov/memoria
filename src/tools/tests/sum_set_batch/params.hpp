
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_SUM_SET_BATCH_PARAMS_HPP_
#define MEMORIA_TESTS_SUM_SET_BATCH_PARAMS_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>

#include "../shared/params.hpp"

namespace memoria {


struct SumSetBatchReplay: public ReplayParams {

	Int 	data_;
	bool 	insert_;
	Int		block_size_;

	Int		page_step_;

	BigInt 	pos_;

	Int 	cnt_;

	SumSetBatchReplay(): ReplayParams(), data_(0), insert_(true), block_size_(0), page_step_(-1), pos_(-1), cnt_(0)
	{
		Add("data", 		data_);
		Add("insert", 		insert_);
		Add("block_size", 	block_size_);
		Add("page_step", 	page_step_);
		Add("pos", 			pos_);
		Add("cnt", 			cnt_);
	}
};


struct SumSetBatchParams: public TestTaskParams {

	Int max_block_size_;

	SumSetBatchParams(StringRef name = "SumSetBatch"):
		TestTaskParams(name),
		max_block_size_(1024*40)
	{
		size_ = 1024*1024*16;
		Add("max_block_size", max_block_size_);
	}
};


}


#endif
