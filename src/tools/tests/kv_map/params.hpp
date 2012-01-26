
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


class KVMapReplay: public TestReplayParams {

	Int step_;
	Int vector_idx_;
	Int size_;

	String pairs_data_file_;
	String pairs_sorted_data_file_;

public:
	KVMapReplay(StringRef name = "KVMapReplay"): TestReplayParams(name), step_(0), vector_idx_(0), size_(0)
	{
		Add("step", step_);
		Add("vectorIdx", vector_idx_);
		Add("size", size_);
		Add("pairsDataFile", pairs_data_file_);
		Add("pairsSortedDataFile", pairs_sorted_data_file_);
	}

	virtual ~KVMapReplay() throw () {};


	Int GetStep() const
	{
		return step_;
	}

	void SetStep(Int step)
	{
		this->step_ = step;
	}

	Int GetVectorIdx() const
	{
		return vector_idx_;
	}

	void SetVectorIdx(Int vector_idx)
	{
		this->vector_idx_ = vector_idx;
	}

	Int GetSize() const
	{
		return size_;
	}

	void SetSize(Int size)
	{
		this->size_ = size;
	}

	StringRef GetPairsDataFile() const
	{
		return pairs_data_file_;
	}

	void SetPairsDataFile(StringRef file)
	{
		this->pairs_data_file_ = file;
	}

	StringRef GetPairsSortedDataFile() const
	{
		return pairs_sorted_data_file_;
	}

	void SetPairsSortedDataFile(StringRef file)
	{
		this->pairs_sorted_data_file_ = file;
	}
};


class KVMapTestTaskParams: public TaskParametersSet {

	Int size_;

public:
	KVMapTestTaskParams(): TaskParametersSet("KVMap")
	{
		Add("size", size_, 1024);
	}

	Int GetSize() const
	{
		return size_;
	}
};


}


#endif
