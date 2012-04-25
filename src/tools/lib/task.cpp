
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <memoria/tools/task.hpp>
#include <memoria/tools/examples.hpp>
#include <memoria/tools/benchmarks.hpp>
#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>
#include <memoria/core/tools/file.hpp>

#include <algorithm>

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





void BenchmarkTask::Run(ostream& out)
{
	BenchmarkParams* params = this->GetParameters<BenchmarkParams>();

	BenchmarkResult sum(this->GetGraphName());

	sum.runs() = params->count;

	BigInt start = GetTimeInMillis();

	for (Int c = 0; c < params->count; c++)
	{
		BenchmarkResult result(sum.name());
		this->Benchmark(result, out);

		sum += result;

		sum.x() = result.x();
	}

	sum.time() = GetTimeInMillis() - start;

	if (this->group_->time())
	{
		sum.value() = sum.time();
	}
	else {
		sum.value() = sum.operations();
	}

	this->group_->results().push_back(sum);
}


Int BenchmarkRunner::Run(ostream& out)
{
	BigInt total_start = GetTimeInMillis();

	Int passed = 0;

	for (Int c = 0; c < GetRunCount(); c++)
	{
		out<<"Pass "<<(c + 1)<<" of "<<GetRunCount()<<endl;

		for (auto igroup = groups_.begin(); igroup != groups_.end(); igroup++)
		{
			BenchmarkGroup* group = *igroup;

			if (group->IsEnabled())
			{
				String group_folder = GetTaskOutputFolder(group->name(), c + 1);
				group->output_folder() = group_folder;

				BigInt start = GetTimeInMillis();

				for (auto itask = group->tasks().begin(); itask != group->tasks().end(); itask++)
				{
					BenchmarkTask* task = *itask;

					String out_file_name = group_folder + Platform::GetFilePathSeparator() + task->GetTaskName()+"-output.txt";

					task->SetOutputFolder(group_folder);

					try {

						File folder(group_folder);
						if (!folder.MkDirs())
						{
							throw MemoriaException(MEMORIA_SOURCE, "Can't create folder: "+group_folder);
						}

						for (Int time = 0; time <task->times(); time++)
						{
							if (!TaskRunner::Run(task, out_file_name, out, time))
							{
								break;
							}
						}
					}
					catch (fstream::failure e)
					{
						out << "Exception opening/writing file: "+out_file_name;
						break;
					}
				}

				group->duration() = GetTimeInMillis() - start;

				// build gnuplot script

				String gnuplot_file_name = group_folder + Platform::GetFilePathSeparator() + "gnuplot.plot";

				BuildGnuplotScript(group, gnuplot_file_name);

				passed++;
			}
		}
	}

	out<<"----------------------------------------------"<<endl;
	for (auto i = groups_.begin(); i != groups_.end(); i++)
	{
		BenchmarkGroup* group = *i;
		if (group->IsEnabled())
		{
			out<<"Total Time for "<<group->name()<<": "<<FormatTime(group->duration())<<endl;
		}
	}

	out<<"Done: "<<passed<<" of "<<groups_.size()<<endl;

	out<<"Total execution time: "<<(FormatTime(GetTimeInMillis() - total_start))<<endl;

	return passed;
}


void BenchmarkRunner::BuildGnuplotScript(BenchmarkGroup* group, StringRef file_name)
{
	typedef vector<BenchmarkResult> 		Results;
	typedef pair<String, Results>			GraphPair;

	vector<GraphPair> graphs;

	for (BenchmarkTask* task: group->tasks())
	{
		Results results;
		String name = task->GetGraphName();

		for (BenchmarkResult& result: group->results())
		{
			if (result.name() == name)
			{
				results.push_back(result);
			}
		}

		graphs.push_back(GraphPair(name, results));
	}

	for (auto& graph: graphs)
	{
		std::sort(graph.second.begin(), graph.second.end());
	}

	fstream out_file;
	out_file.exceptions ( fstream::failbit | fstream::badbit );
	out_file.open(file_name, fstream::out);

	out_file<<"set terminal png size "<<group->resolution()<<endl;
	out_file<<"set output '"+group->name()+".png'"<<endl;
	out_file<<"set title \""+group->title()+"\""<<endl;
	out_file<<"set xlabel \""+group->xtitle()+"\""<<endl;
	out_file<<"set ylabel \""+group->ytitle()+"\""<<endl;
	out_file<<"set logscale x "<<group->logscale()<<endl;
	out_file<<"set key "<<group->agenda_location()<<endl;

	out_file<<"plot ";

	Int cnt = 0;
	for (auto& graph: graphs)
	{
		out_file<<"'-' title '"+graph.first<<"' w l";

		if (cnt++ < (Int)graphs.size() - 1)
		{
			out_file<<", ";
		}
	}

	out_file<<endl;

	for (auto& graph: graphs)
	{
		for (auto& result: graph.second)
		{
			out_file<<result.x()<<" "<<result.plot_value()<<endl;
		}

		out_file<<"e"<<endl;
	}

	out_file.close();
}


void BenchmarkRunner::Configure(Configurator* cfg)
{
	for (auto igroup = groups_.begin(); igroup != groups_.end(); igroup++)
	{
		BenchmarkGroup* group = *igroup;
		group->Process(cfg);
	}

	TaskRunner::Configure(cfg);
}

}
