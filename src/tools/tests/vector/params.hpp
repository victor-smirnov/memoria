
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
	Int		block_size_;

	Int		page_step_;

	BigInt 	pos_;

	Int 	cnt_;

	BigInt ctr_name_;

public:
	VectorReplay(): ReplayParams(), data_(0), insert_(true), block_size_(0), page_step_(-1), pos_(-1), cnt_(0)
	{
		Add("data", 		data_);
		Add("insert", 		insert_);
		Add("data_size", 	block_size_);
		Add("page_step", 	page_step_);
		Add("pos", 			pos_);
		Add("cnt", 			cnt_);
		Add("ctr_name", 	ctr_name_);
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
