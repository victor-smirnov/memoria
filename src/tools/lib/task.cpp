
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
	for (Task* t: tasks_)
	{
		delete t;
	}
}

void TaskRunner::Configure(Configurator* cfg)
{
	for (Task* t: tasks_)
	{
		t->GetParameters()->SetCfg(cfg);
	}
}

void TaskRunner::Run(ostream& out)
{
	for (Task* t: tasks_)
	{
		if (t->GetParameters()->IsEnabled())
		{
			t->Run(out);
		}
	}
}

void TaskRunner::DumpProperties(ostream& out)
{
	for (Task* t: tasks_)
	{
		t->GetParameters()->DumpProperties(out);
	}
}


}
