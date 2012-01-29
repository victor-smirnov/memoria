
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

class TemplateReplay: public TestReplayParams {

	Int param_;

public:
	TemplateReplay(StringRef name = "Replay"): TestReplayParams(name), param_(0)
	{
		Add("param", param_);
	}

	virtual ~TemplateReplay() throw () {};

	Int GetParam() const {
		return param_;
	}

	void SetParam(Int param)
	{
		param_ = param;
	}

private:
};


class TemplateParams: public TaskParametersSet {

	Int size_;

public:
	TemplateParams(): TaskParametersSet("Template")
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

	TemplateTestTask(): SPTestTask(new TemplateParams()) {}

	virtual ~TemplateTestTask() throw() {}

	virtual TestReplayParams* CreateTestStep(StringRef name) const
	{
		return new TemplateReplay(name);
	}

	virtual void Replay(ostream& out, TestReplayParams* step_params)
	{
		TemplateReplay* params = static_cast<TemplateReplay*>(step_params);
		Allocator allocator;
		LoadAllocator(allocator, params);

		DoTestStep(out, allocator, params);
	}

	virtual void Run(ostream& out)
	{
		TemplateReplay params;
		out<<GetTaskName()<<": "<<"Do main things"<<endl;

		Allocator allocator;

		try {
			DoTestStep(out, allocator, &params);
		}
		catch (...) {
			Store(allocator, &params);
			throw;
		}
	}

	void DoTestStep(ostream& out, Allocator& allocator, const TemplateReplay* params)
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
