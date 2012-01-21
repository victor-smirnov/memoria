
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <memoria/tools/tests.hpp>

#include <iostream>

namespace memoria {

using namespace std;
using namespace memoria::vapi;

void TestTask::Replay(ostream& out, Configurator* cfg)
{
	unique_ptr<TestStepParams> params(cfg != NULL ? ReadTestStep(cfg) : NULL);
	Replay(out, params.get());
}

String TestTask::GetFileName(StringRef name) const
{
	return name + ".properties";
}

TestStepParams* TestTask::ReadTestStep(Configurator* cfg) const
{
	String name = cfg->GetProperty("name");
	TestStepParams* params = CreateTestStep(name);
	Configure(params);

	params->Process(cfg);

	return params;
}

void TestTask::Configure(TestStepParams* params) const
{
	params->SetTask(GetTaskName());
	params->SetReplay(true);
}

}
