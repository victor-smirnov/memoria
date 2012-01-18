
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_TEMPLATE_TASK_HPP_
#define MEMORIA_TESTS_TEMPLATE_TASK_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>


namespace memoria {

using namespace memoria::vapi;

class TemplateTestStepParams: public TestStepParams {

	Int param_;

public:
	TemplateTestStepParams(StringRef name = "KVMap"): TestStepParams(name), param_(0)
	{
		Add("param", param_);
	}

	virtual ~TemplateTestStepParams() throw () {};

	Int GetParam() const {
		return param_;
	}

	void SetParam(Int param)
	{
		param_ = param;
	}

private:
};


class TemplateTestTaskParams: public TaskParametersSet {

	Int size_;

public:
	TemplateTestTaskParams(): TaskParametersSet("TemplateTestTask")
	{
		Add("size", size_, 1024);
		SetEnabled(false);
	}

	Int GetSize() const
	{
		return size_;
	}
};


class TemplateTestTask: public SPTestTask {

public:

	TemplateTestTask(): SPTestTask(new TemplateTestTaskParams()) {}

	virtual ~TemplateTestTask() throw() {}

	virtual TestStepParams* CreateTestStep(StringRef name) const
	{
		return new TemplateTestStepParams(name);
	}

	virtual void Run(ostream& out, TestStepParams* step_params)
	{
		if (step_params != NULL)
		{
			KVMapTestStepParams* params = static_cast<KVMapTestStepParams*>(step_params);
			Allocator allocator;
			LoadAllocator(allocator, params);

			DoTestStep(out, allocator, params);
		}
		else {
			KVMapTestStepParams params;
			out<<GetTaskName()<<": "<<"Do main things"<<endl;

			Allocator allocator;

			try {
				DoTestStep(out, allocator, &params);
			}
			catch (...) {
				Store(allocator, &params);
			}
		}
	}

	void DoTestStep(ostream& out, Allocator& allocator, const KVMapTestStepParams* params)
	{
		if (!params->IsReplay()) {
			throw MemoriaException(MEMORIA_SOURCE, "Test Exception");
		}
		else {
			out<<"Replay is done"<<endl;
		}
	}
};


}


#endif
