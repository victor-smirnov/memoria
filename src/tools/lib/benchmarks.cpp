
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <memoria/tools/task.hpp>
#include <memoria/tools/benchmarks.hpp>

#include <algorithm>
#include <fstream>

namespace memoria {

BenchmarkTaskGroup::~BenchmarkTaskGroup() throw ()
{
}


void BenchmarkTaskGroup::Run(ostream& out)
{
	for (auto t: tasks_)
	{
		BenchmarkTask* task = T2T_S<BenchmarkTask*>(t);
		try {

			task->SetIteration(0);
			task->SetOutputFolder(GetOutputFolder());

			task->BuildResources();

			ResetTime();

			while (!IsEnd())
			{
				BenchmarkParameters result(task->GetGraphName());
				result.x() 			= NextTime();
				result.operations()	= this->operations;
				result.xunit()		= this->xunit;

				task->Prepare(result);

				BigInt start = GetTimeInMillis();

				for (Int c = 0; c < task->average; c++)
				{
					task->Benchmark(result);
				}

				BigInt end = GetTimeInMillis();

				result.time() = (end - start) / task->GetAverage();

				BigInt x 	= (result.x() / result.xunit());
				BigInt time = (result.time() / result.yunit());

				out<<task->GetFullName()<<": "<<x<<" "<<time<<endl;
				cout<<task->GetFullName()<<": "<<x<<" "<<time<<endl;

				results_.push_back(result);

				task->Release();
			}

			out<<endl;
			cout<<endl;

		}
		catch (...) {
			task->ReleaseResources();
		}
	}
}


void BenchmarkTaskGroup::RegisterTask(BenchmarkTask* task)
{
	for (auto t: tasks_)
	{
		if (T2T_S<BenchmarkTask*>(t)->GetGraphName() == task->GetGraphName())
		{
			throw MemoriaException(MEMORIA_SOURCE, "Duplicate graph name" + task->GetGraphName());
		}
	}

	TaskGroup::RegisterTask(task);
}





void GnuplotGraph::Run(ostream& out)
{
	BenchmarkTaskGroup::Run(out);

	BuildGnuplotScript(GetOutputFolder() + Platform::GetFilePathSeparator() + GetTaskName()+".plot");
}

void GnuplotGraph::BuildGnuplotScript(StringRef file_name)
{
	typedef vector<BenchmarkParameters> 		Results;
	typedef pair<String, Results>			GraphPair;

	vector<GraphPair> graphs;

	for (auto t: tasks_)
	{
		BenchmarkTask* task = T2T_S<BenchmarkTask*>(t);

		Results results;
		String name = task->GetGraphName();

		for (BenchmarkParameters& result: results_)
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

	out_file<<"set terminal png size "<<this->resolution<<endl;
	out_file<<"set output '"+this->GetTaskName()+".png'"<<endl;
	out_file<<"set title \""+this->title+"\""<<endl;
	out_file<<"set xlabel \""+this->xtitle+"\""<<endl;
	out_file<<"set ylabel \""+this->ytitle+"\""<<endl;

	if (this->logscale > 0)
	{
		out_file<<"set logscale x "<<this->logscale<<endl;
	}

	out_file<<"set key "<<this->agenda_location<<endl;

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
			out_file<<(result.x() / result.xunit())<<" "<<result.plot_value()<<endl;
		}

		out_file<<"e"<<endl;
	}

	out_file.close();
}















//
//Int BenchmarkRunner::Run(ostream& out)
//{
//	BigInt total_start = GetTimeInMillis();
//
//	Int passed = 0;
//
//	for (Int c = 0; c < GetRunCount(); c++)
//	{
//		out<<"Pass "<<(c + 1)<<" of "<<GetRunCount()<<endl;
//
//		for (auto igroup = groups_.begin(); igroup != groups_.end(); igroup++)
//		{
//			BenchmarkGroup* group = *igroup;
//
//			if (group->IsEnabled())
//			{
//				String group_folder = GetTaskOutputFolder(group->name(), c + 1);
//				group->output_folder() = group_folder;
//
//				BigInt start = GetTimeInMillis();
//
//				for (auto itask = group->tasks().begin(); itask != group->tasks().end(); itask++)
//				{
//					BenchmarkTask* task = *itask;
//
//					String out_file_name = group_folder + Platform::GetFilePathSeparator() + task->GetTaskName()+"-output.txt";
//
//					task->SetOutputFolder(group_folder);
//
//					try {
//
//						File folder(group_folder);
//						if (!folder.MkDirs())
//						{
//							throw MemoriaException(MEMORIA_SOURCE, "Can't create folder: "+group_folder);
//						}
//
//						for (Int time = 0; time <task->times(); time++)
//						{
//							if (!TaskRunner::Run(task, out_file_name, out, time))
//							{
//								break;
//							}
//						}
//					}
//					catch (fstream::failure e)
//					{
//						out << "Exception opening/writing file: "+out_file_name;
//						break;
//					}
//				}
//
//				group->duration() = GetTimeInMillis() - start;
//
//				// build gnuplot script
//
//				String gnuplot_file_name = group_folder + Platform::GetFilePathSeparator() + "gnuplot.plot";
//
//				BuildGnuplotScript(group, gnuplot_file_name);
//
//				passed++;
//			}
//		}
//	}
//
//	out<<"----------------------------------------------"<<endl;
//	for (auto i = groups_.begin(); i != groups_.end(); i++)
//	{
//		BenchmarkGroup* group = *i;
//		if (group->IsEnabled())
//		{
//			out<<"Total Time for "<<group->name()<<": "<<FormatTime(group->duration())<<endl;
//		}
//	}
//
//	out<<"Done: "<<passed<<" of "<<groups_.size()<<endl;
//
//	out<<"Total execution time: "<<(FormatTime(GetTimeInMillis() - total_start))<<endl;
//
//	return passed;
//}
//
//
//void BenchmarkRunner::BuildGnuplotScript(BenchmarkGroup* group, StringRef file_name)
//{
//	typedef vector<BenchmarkResult> 		Results;
//	typedef pair<String, Results>			GraphPair;
//
//	vector<GraphPair> graphs;
//
//	for (BenchmarkTask* task: group->tasks())
//	{
//		Results results;
//		String name = task->GetGraphName();
//
//		for (BenchmarkResult& result: group->results())
//		{
//			if (result.name() == name)
//			{
//				results.push_back(result);
//			}
//		}
//
//		graphs.push_back(GraphPair(name, results));
//	}
//
//	for (auto& graph: graphs)
//	{
//		std::sort(graph.second.begin(), graph.second.end());
//	}
//
//	fstream out_file;
//	out_file.exceptions ( fstream::failbit | fstream::badbit );
//	out_file.open(file_name, fstream::out);
//
//	out_file<<"set terminal png size "<<group->resolution()<<endl;
//	out_file<<"set output '"+group->name()+".png'"<<endl;
//	out_file<<"set title \""+group->title()+"\""<<endl;
//	out_file<<"set xlabel \""+group->xtitle()+"\""<<endl;
//	out_file<<"set ylabel \""+group->ytitle()+"\""<<endl;
//	out_file<<"set logscale x "<<group->logscale()<<endl;
//	out_file<<"set key "<<group->agenda_location()<<endl;
//
//	out_file<<"plot ";
//
//	Int cnt = 0;
//	for (auto& graph: graphs)
//	{
//		out_file<<"'-' title '"+graph.first<<"' w l";
//
//		if (cnt++ < (Int)graphs.size() - 1)
//		{
//			out_file<<", ";
//		}
//	}
//
//	out_file<<endl;
//
//	for (auto& graph: graphs)
//	{
//		for (auto& result: graph.second)
//		{
//			out_file<<result.x()<<" "<<result.plot_value()<<endl;
//		}
//
//		out_file<<"e"<<endl;
//	}
//
//	out_file.close();
//}
//
//
//void BenchmarkRunner::Configure(Configurator* cfg)
//{
//	for (auto igroup = groups_.begin(); igroup != groups_.end(); igroup++)
//	{
//		BenchmarkGroup* group = *igroup;
//		group->Process(cfg);
//	}
//
//	TaskRunner::Configure(cfg);
//}
//


}
