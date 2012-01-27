
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <memoria/tools/task.hpp>
#include <memoria/tools/tools.hpp>
#include <memoria/core/tools/file.hpp>

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

void TaskRunner::Replay(ostream& out, StringRef replay_file)
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
	Task* task = GetTask<Task*>(name);
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

void TaskRunner::Run(ostream& out)
{
	BigInt total_start = GetTimeInMillis();

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

				fstream out_file;

				try {

					File folder(task_folder);
					if (!folder.MkDirs())
					{
						throw MemoriaException(MEMORIA_SOURCE, "Can't create folder: "+task_folder);
					}

					out_file.exceptions ( fstream::failbit | fstream::badbit );
					out_file.open(out_file_name, fstream::out);

					BigInt start = GetTimeInMillis();
					try {
						out<<"Task: "<<t->GetTaskName()<<" ";
						out_file<<"Task: "<<t->GetTaskName()<<endl;

						t->SetIteration(c);
						t->Run(out_file);

						out<<"PASSED ";
						out_file<<"PASSED"<<endl;
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

					t->SetDuration(t->GetDuration() + stop - start);

					String total_task_time = FormatTime(stop - start);

					out<<" Duration: "<<total_task_time<<endl;
					out_file<<"Duration: "<<total_task_time<<endl;

					out_file.close();
				}
				catch (fstream::failure e)
				{
					out << "Exception opening/writing file: "+out_file_name;
				}
			}
		}
		cout<<endl;
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
