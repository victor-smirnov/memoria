
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_TREE_MAP_HPP_
#define MEMORIA_TESTS_TREE_MAP_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>


namespace memoria {

class KVMapTestStepParams: public TestStepParams {

	Int param_;

public:
	KVMapTestStepParams(): TestStepParams(), param_(0)
	{
		Add("param", param_);
	}

	virtual ~KVMapTestStepParams() throw () {};

private:
};


class TreeMapTestTaskParams: public TaskParametersSet {

	Int size_;

public:
	TreeMapTestTaskParams(): TaskParametersSet("KVMap")
	{
		Add("size", size_, 1024);
	}

	Int GetSize() const
	{
		return size_;
	}
};


class KVMapTestTask: public SPTestTask {

//	Allocator allocator_;

public:

	KVMapTestTask(): SPTestTask(new TreeMapTestTaskParams()) {}

	virtual ~KVMapTestTask() throw() {}

//	virtual Allocator& GetAllocator()
//	{
//		return allocator_;
//	}

//	virtual void ExecuteTask(ostream& out)
//	{
//		TreeMapTestTaskParametersSet* params = GetParameters<TreeMapTestTaskParametersSet>();
//
//		out<<"size="<<params->GetSize()<<endl;
//		out<<"Task "<<GetTaskName()<<" has been executed"<<endl;
//
//		CreateKVMapUpdateOp op;
//
//		RunOp(&op);
//	}

	virtual TestStepParams* CreateTestStep(StringRef name)
	{
		return new KVMapTestStepParams();
	}

	virtual void Run(ostream& out, TestStepParams* step_params)
	{
		out<<"KVMapTestTask: "<<step_params<<endl;
	}
};


}


#endif
