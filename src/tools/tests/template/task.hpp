
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_TEMPLATE_TASK_HPP_
#define MEMORIA_TESTS_TEMPLATE_TASK_HPP_

#include <memoria/memoria.hpp>

#include "../tests_inc.hpp"



namespace memoria {

using namespace memoria::vapi;




class TemplateTestTask: public SPTestTask {

	Int param;

public:

	struct TestReplay: public TestReplayParams
	{
		Int replay_param;

		TestReplay(): TestReplayParams(), replay_param(0)
		{
			Add("replay_param", replay_param);
		}
	};



	TemplateTestTask(): SPTestTask("Test"), param(1024)
	{
		Add("param", param);
	}

	virtual ~TemplateTestTask() throw() {}

	virtual TestReplayParams* CreateTestStep(StringRef name) const
	{
		return new TestReplay();
	}

	virtual void Replay(ostream& out, TestReplayParams* step_params)
	{
		TestReplay* params = static_cast<TestReplay*>(step_params);
		Allocator allocator;

		LoadAllocator(allocator, params);

		//do something with allocator
	}

	virtual void Run(ostream& out)
	{
		TestReplay params;
		out<<getTaskName()<<": "<<"Do main things"<<endl;

		Allocator allocator;
		allocator.commit();

		try {
			throw Exception(MEMORIA_SOURCE, "Test Exception");
		}
		catch (...) {
			Store(allocator, &params);
			throw;
		}
	}
};

class TemplateTestSuite: public TestSuite {
public:

	TemplateTestSuite(): TestSuite("Template")
	{
		RegisterTask(new TemplateTestTask());
	}
};


}


#endif
