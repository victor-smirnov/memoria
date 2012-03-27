
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
	unique_ptr<TestReplayParams> params(cfg != NULL ? ReadTestStep(cfg) : NULL);
	Replay(out, params.get());
}

String TestTask::GetFileName(StringRef name) const
{
	return name + ".properties";
}

TestReplayParams* TestTask::ReadTestStep(Configurator* cfg) const
{
	String name = cfg->GetProperty("name");
	TestReplayParams* params = CreateTestStep(name);
	Configure(params);

	params->Process(cfg);

	return params;
}

void TestTask::Configure(TestReplayParams* params) const
{
	params->SetTask(GetTaskName());
	params->SetReplay(true);
}


void TestRunner::Replay(ostream& out, StringRef replay_file)
{
	File file(replay_file);

	String file_name;

	if (file.IsDirectory())
	{
		file_name = replay_file + Platform::GetFilePathSeparator() + "Replay.properties";
	}
	else if (!file.IsExists())
	{
		throw MemoriaException(MEMORIA_SOURCE, "File "+replay_file +" does not exists");
	}

	Configurator cfg;
	Configurator::Parse(file_name.c_str(), &cfg);

	String name = cfg.GetProperty("task");
	TestTask* task = GetTask<TestTask*>(name);
	try {
		out<<"Task: "<<task->GetTaskName()<<endl;
		task->Replay(out, &cfg);
		out<<"PASSED"<<endl;
	}
	catch (MemoriaException e)
	{
		out<<"FAILED: "<<e.source()<<" "<<e.message()<<endl;
	}
	catch (...)
	{
		out<<"FAILED"<<endl;
	}
}



}
