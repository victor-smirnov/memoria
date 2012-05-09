
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <memoria/tools/tests.hpp>


#include <algorithm>
#include <fstream>
#include <memory>

namespace memoria {


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
	params->SetTask(GetFullName());
	params->SetReplay(true);
}



void MemoriaTestRunner::Replay(ostream& out, StringRef task_folder)
{
	File folder(task_folder);

	Configurator cfg;
	Configurator task_cfg;

	String replay_file_name;
	String task_file_name;

	if (folder.IsExists())
	{
		replay_file_name = task_folder + Platform::GetFilePathSeparator() + "Replay.properties";
		File file(replay_file_name);

		if (!file.IsExists())
		{
			throw MemoriaException(MEMORIA_SOURCE, "File " + replay_file_name +" does not exists");
		}

		task_file_name = task_folder + Platform::GetFilePathSeparator() + "ReplayTask.properties";
		File task_file(task_file_name);

		if (!file.IsExists())
		{
			throw MemoriaException(MEMORIA_SOURCE, "File " + task_file_name +" does not exists");
		}
	}
	else {
		throw MemoriaException(MEMORIA_SOURCE, "File " + task_folder + " does not exists");
	}


	Configurator::Parse(replay_file_name.c_str(), &cfg);

	String name = cfg.GetProperty("task");
	TestTask* task = GetTask<TestTask>(name);
	if (task != NULL)
	{
		try {
			out<<"Task: "<<task->GetFullName()<<endl;
			task->LoadProperties(task, task_file_name);
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
	else {
		out<<"FAILED: Task '"<<name<<"' is not found"<<endl;
	}
}




}
