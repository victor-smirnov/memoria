
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <memoria/tools/task.hpp>

namespace memoria {

using namespace std;

Task::~Task() throw () {
	try {
		delete parameters_;
	}
	catch (...) {

	}
}

TaskRunner::~TaskRunner()
{
	for (auto i = tasks_.begin(); i != tasks_.end(); i++)
	{
		delete i->second;
	}
}

void TaskRunner::Configure(Configurator* cfg)
{
	for (auto i = tasks_.begin(); i != tasks_.end(); i++)
	{
		i->second->GetParameters()->Process(cfg);
	}
}

void TaskRunner::Run(ostream& out, Configurator* cfg)
{
	if (cfg == NULL)
	{
		for (auto i = tasks_.begin(); i != tasks_.end(); i++)
		{
			Task* t = i->second;

			if (t->GetParameters()->IsEnabled())
			{
				t->Run(out, NULL);
			}
		}
	}
	else
	{
		String name = cfg->GetProperty("task");
		Task* task = GetTask<Task*>(name);
		task->Run(out, cfg);
	}
}

void TaskRunner::DumpProperties(ostream& out)
{
	for (auto i = tasks_.begin(); i != tasks_.end(); i++)
	{
		i->second->GetParameters()->DumpProperties(out);
	}
}


}
