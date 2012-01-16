
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_TREE_MAP_HPP_
#define MEMORIA_TESTS_TREE_MAP_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>


namespace memoria {

class CreateKVMapUpdateOp: public SPUpdateOp {

	Int param_;

public:
	CreateKVMapUpdateOp(): SPUpdateOp("CreateKVMap"), param_(0) {InitProperties();}
	CreateKVMapUpdateOp(Int param):SPUpdateOp("CreateKVMap"), param_(param) {InitProperties();}

	virtual ~CreateKVMapUpdateOp() throw () {};

	virtual void Run()
	{
		cout<<"Execute UpdateOp: "<<GetName()<<endl;

		throw MemoriaException(MEMORIA_SOURCE, "Fake!");
	}
private:

	void InitProperties()
	{
		Add("param", param_);
	}
};


class TreeMapTestTaskParametersSet: public ParametersSet {

	Int size_;

public:
	TreeMapTestTaskParametersSet(): ParametersSet("TreeMap")
	{
		Add("size", size_, 1024);
	}

	Int GetSize() const
	{
		return size_;
	}
};


class TreeMapTestTask: public SPTestTask {

	Allocator allocator_;

public:

	TreeMapTestTask(): SPTestTask(new TreeMapTestTaskParametersSet())
	{
		RegisterUpdateOp(new CreateKVMapUpdateOp());
	}

	virtual Allocator& GetAllocator()
	{
		return allocator_;
	}

	virtual void ExecuteTask(ostream& out)
	{
		TreeMapTestTaskParametersSet* params = static_cast<TreeMapTestTaskParametersSet*>(GetParameters());

		out<<"size="<<params->GetSize()<<endl;
		out<<"Task "<<GetTaskName()<<" has been executed"<<endl;

		CreateKVMapUpdateOp op;

		RunOp(&op);
	}
};


}


#endif
