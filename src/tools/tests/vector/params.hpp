
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

class VectorTestStepParams: public TestStepParams {
public:
	Int 	step_;
	Int 	data_;
	bool 	insert_;
	Int		data_size_;

public:
	VectorTestStepParams(StringRef name = "Vector"): TestStepParams(name), step_(0), data_(0), insert_(true), data_size_(0)
	{
		Add("step", step_);
		Add("data", data_);
		Add("insert", insert_);
		Add("dataSize", data_size_);
	}

	virtual ~VectorTestStepParams() throw () {};

private:
};


class VectorTestTaskParams: public TaskParametersSet {

public:
	Int size_;
	Int max_block_size_;

public:
	VectorTestTaskParams(): TaskParametersSet("VectorTestTask")
	{
		Add("size", size_, 1024*1024*16);
		Add("maxBlockSize", max_block_size_, 1024*40);
	}
};



}


#endif
