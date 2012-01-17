
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <memoria/tools/tests.hpp>

#include <iostream>

namespace memoria {

using namespace std;
using namespace memoria::vapi;

void TestTask::Run(ostream& out, Configurator* cfg)
{
	TestStepParams* params = cfg != NULL ? ReadTestStep(cfg) : NULL;
	Run(out, params);
}

String TestTask::GetFileName(StringRef name)
{
	return name + ".properties";
}

TestStepParams* TestTask::ReadTestStep(Configurator* cfg)
{
	String name = cfg->GetProperty("name");
	TestStepParams* params = CreateTestStep(name);
	params->Process(cfg);

	return params;
}

}
