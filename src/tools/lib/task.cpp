
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <memoria/tools/task.hpp>
#include <memoria/tools/tools.hpp>
#include <memoria/core/tools/file.hpp>

#include <algorithm>
#include <memory>

namespace memoria {

using namespace std;

Task::~Task() throw () {

}


void Task::Configure(Configurator* cfg)
{
	TaskParametersSet* params = GetParameters<>();

	if (constext_name_ != "")
	{
		params->SetPrefix(constext_name_ + "." + params->GetPrefix());
	}

	params->Process(cfg);
}


void Task::BuildResources()
{
	File output_f(output_folder_);

	bool own_folder = this->own_folder;

	if (!output_f.IsExists())
	{
		output_f.MkDirs();
	}

	String out_file_name = output_folder_ + Platform::GetFilePathSeparator() + (own_folder ? "" : GetTaskName() + ".") + "output.txt";

	out_ = new fstream();
	out_->exceptions ( fstream::failbit | fstream::badbit );
	out_->open(out_file_name, fstream::out);
}

void Task::ReleaseResources()
{
	out_->close();
	delete out_;
}


Int Task::Run()
{
	BuildResources();

	bool result;
	try {
		Run(*out_);

		(*out_)<<"PASSED"<<endl;
		cout<<GetFullName()<<" PASSED"<<endl;

		result = true;
	}
	catch (...)
	{
		(*out_)<<"FAILED"<<endl;
		cout<<GetFullName()<<" FAILED"<<endl;
		result = false;
	}

	ReleaseResources();

	return result;
}



TaskGroup::~TaskGroup() throw ()
{
	try {
		for (auto t: tasks_)
		{
			delete t;
		}
	}
	catch (...) {}
}

void TaskGroup::Run(ostream& out)
{
	for (auto t: tasks_)
	{
		String folder;

		if (t->own_folder)
		{
			folder = output_folder_ + Platform::GetFilePathSeparator() + t->GetTaskName();
		}
		else {
			folder = output_folder_;
		}

		t->SetOutputFolder(folder);
		t->SetIteration(1);
		t->Run();
	}
}

void TaskGroup::RegisterTask(Task* task)
{
	for (auto t: tasks_)
	{
		if (t->GetTaskName() == task->GetTaskName())
		{
			throw MemoriaException(MEMORIA_SOURCE, "Task " + task->GetTaskName()+" is already registered");
		}
	}

	tasks_.push_back(task);
}

void TaskGroup::Configure(Configurator* cfg)
{
	Task::Configure(cfg);

	for (auto t: tasks_)
	{
		t->SetContextName(this->GetFullName());
		t->Configure(cfg);
	}
}

void TaskGroup::BuildResources()
{
	Task::BuildResources();
}

void TaskGroup::ReleaseResources()
{
	Task::ReleaseResources();
}



Int GroupRunner::Run()
{
	BuildResources();

	Int counter = 0;

	Int run_count = GetRunCount();

	for (Int c = 1; c <= run_count; c++)
	{
		String task_folder;
		if (run_count > 1)
		{
			String folder_name = "run-" + ToString(c);
			task_folder = output_folder_ + Platform::GetFilePathSeparator() + folder_name;
		}
		else {
			task_folder = output_folder_;
		}

		File folder(task_folder);
		if (!folder.IsExists())
		{
			folder.MkDirs();
		}

		for (auto t: tasks_)
		{
			if (t->IsEnabled())
			{
				String folder;

				if (t->own_folder)
				{
					folder = task_folder + Platform::GetFilePathSeparator() + t->GetTaskName();
				}
				else {
					folder = task_folder;
				}

				t->SetOutputFolder(folder);
				t->SetIteration(c);
				counter += t->Run();
			}
		}
	}

	ReleaseResources();

	return counter;
}





TaskRunner::~TaskRunner()
{
	for (auto task_p: tasks_)
	{
		delete task_p.second;
	}
}

void TaskRunner::Configure(Configurator* cfg)
{
	for (auto task_p: tasks_)
	{
		task_p.second->Configure(cfg);
	}
}


bool TaskRunner::Run(Task* task, StringRef out_file_name, ostream& out, Int c)
{
	fstream out_file;

	bool passed = false;

	try {
		out_file.exceptions ( fstream::failbit | fstream::badbit );
		out_file.open(out_file_name, fstream::out);

		BigInt start = GetTimeInMillis();
		try {
			out<<"Task: "<<task->GetTaskName()<<" ";
			out_file<<"Task: "<<task->GetTaskName()<<endl;

			task->SetIteration(c);

			task->Prepare(out_file);
			task->Run(out_file);
			task->Release(out_file);

			passed = true;
			out<<"PASSED ";
			out_file<<"PASSED"<<endl;
		}
		catch (MemoriaSigSegv e)
		{
			out<<"SigSegv ";
			out_file<<"FAILED: SigSegv: "<<e.source()<<" "<<e.message()<<endl;
			exit(1);
		}
		catch (MemoriaException e)
		{
			out<<"FAILED ";
			out_file<<"FAILED: "<<e.source()<<" "<<e.message()<<endl;
		}
		catch (ifstream::failure e)
		{
			throw;
		}
		catch (...)
		{
			out<<"FAILED ";
			out_file<<"FAILED"<<endl;
		}

		BigInt stop = GetTimeInMillis();

		task->SetDuration(task->GetDuration() + stop - start);

		String total_task_time = FormatTime(stop - start);

		out<<" Duration: "<<total_task_time<<endl;
		out_file<<"Duration: "<<total_task_time<<endl;

		out_file.close();
	}
	catch (fstream::failure e)
	{
		out << "Exception opening/writing file: "+out_file_name;
	}

	return passed;
}



Int TaskRunner::Run(ostream& out)
{
	BigInt total_start = GetTimeInMillis();

	Int passed = 0;
	Int failed = 0;

	for (Int c = 0; c < GetRunCount(); c++)
	{
		out<<"Pass "<<(c + 1)<<" of "<<GetRunCount()<<endl;

		for (auto i = tasks_.begin(); i != tasks_.end(); i++)
		{
			Task* t = i->second;

			if (t->GetParameters()->IsEnabled())
			{
				String task_folder = GetTaskOutputFolder(t->GetTaskName(), c + 1);
				t->SetOutputFolder(task_folder);

				String out_file_name = task_folder + Platform::GetFilePathSeparator() + "output.txt";

				try {

					File folder(task_folder);
					if (!folder.MkDirs())
					{
						throw MemoriaException(MEMORIA_SOURCE, "Can't create folder: "+task_folder);
					}

					if (Run(t, out_file_name, out, c))
					{
						passed++;
					}
					else {
						failed++;
					}
				}
				catch (fstream::failure e)
				{
					out << "Exception opening/writing file: "+out_file_name;
				}
			}
		}
		out<<endl;
	}

	out<<"----------------------------------------------"<<endl;
	for (auto i = tasks_.begin(); i != tasks_.end(); i++)
	{
		Task* t = i->second;
		if (t->GetParameters()->IsEnabled())
		{
			out<<"Total Time for "<<t->GetTaskName()<<": "<<FormatTime(t->GetDuration())<<endl;
		}
	}

	out<<"Passed: "<<passed<<endl;
	out<<"Failed: "<<failed<<endl;
	out<<"Total execution time: "<<(FormatTime(GetTimeInMillis() - total_start))<<endl;
	return failed;
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
