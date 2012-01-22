
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <memoria/tools/task.hpp>
#include <memoria/tools/tools.hpp>

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

void TaskRunner::Replay(ostream& out, Configurator* cfg)
{
	String name = cfg->GetProperty("task");
	Task* task = GetTask<Task*>(name);
	try {
		out<<"Task: "<<task->GetTaskName()<<endl;
		task->Replay(out, cfg);
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

void TaskRunner::Run(ostream& out)
{
	BigInt total_start = GetTimeInMillis();

	for (auto i = tasks_.begin(); i != tasks_.end(); i++)
	{
		Task* t = i->second;

		if (t->GetParameters()->IsEnabled())
		{
			BigInt start = GetTimeInMillis();
			try {
				out<<"Task: "<<t->GetTaskName()<<endl;
				t->Run(out);
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
			out<<"Execution time: "<<(FormatTime(GetTimeInMillis() - start))<<endl;
		}
	}

	out<<"Total execution time: "<<(FormatTime(GetTimeInMillis() - total_start))<<endl;
}

void TaskRunner::DumpProperties(ostream& out)
{
	for (auto i = tasks_.begin(); i != tasks_.end(); i++)
	{
		out<<"#task: "<<i->second->GetTaskName()<<endl;
		i->second->GetParameters()->DumpProperties(out);
		out<<endl<<endl;
	}
}


}
