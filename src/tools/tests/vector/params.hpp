
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_VECTOR_PARAMS_HPP_
#define MEMORIA_TESTS_VECTOR_PARAMS_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>


namespace memoria {

using namespace memoria::vapi;

class VectorReplay: public ReplayParams {
public:
	Int 	data_;
	bool 	insert_;
	Int		data_size_;

	Int		page_step_;

	BigInt 	pos_;

	Int 	cnt_;

public:
	VectorReplay(): ReplayParams(), data_(0), insert_(true), data_size_(0), page_step_(-1), pos_(-1), cnt_(0)
	{
		Add("data", 	data_);
		Add("insert", 	insert_);
		Add("dataSize", data_size_);
		Add("pageStep", page_step_);
		Add("pos", 		pos_);
		Add("cnt", 		cnt_);
	}
};


class VectorParams: public TestTaskParams {

public:
	Int 	max_block_size_;

public:
	VectorParams(): TestTaskParams("Vector")
	{
		Add("size", size_, 1024*1024*16);
		Add("maxBlockSize", max_block_size_, 1024*40);
	}
};



}


#endif
