
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
				BigInt y 	= (result.plot_value() / result.xunit());

				out<<task->GetFullName()<<": "<<x<<" "<<y<<" ("<<result.time()<<")"<<endl;
				cout<<task->GetFullName()<<": "<<x<<" "<<y<<" ("<<result.time()<<")"<<endl;

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



}
