
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

			task->setIteration(0);
			task->setOutputFolder(getOutputFolder());

			task->BuildResources();

			ResetTime();

			while (!IsEnd())
			{
				BenchmarkParameters result(task->getFullName());

				result.x() 			= NextTime();
				result.operations()	= this->operations;
				result.xunit()		= this->xunit;

				result.yunit()		= this->yunit;
				result.y2unit()		= this->y2unit;

				task->Prepare(result);

				BigInt start = getTimeInMillis();

				for (Int c = 0; c < task->average; c++)
				{
					task->Benchmark(result);
				}

				BigInt end = getTimeInMillis();

				result.time() = (end - start) / task->getAverage();

				BigInt x 	= (result.x() / result.xunit());
				BigInt y1 	= (result.performance() / result.yunit());
				BigInt y2 	= (result.throughput() / result.y2unit());

				out<<task->getFullName()<<": "<<x<<" "<<y1<<" "<<y2<<" ("<<result.time()<<")"<<endl;
				cout<<task->getFullName()<<": "<<x<<" "<<y1<<" "<<y2<<" ("<<result.time()<<")"<<endl;

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
	TaskGroup::RegisterTask(task);
}





void GnuplotGraph::Run(ostream& out)
{
	BenchmarkTaskGroup::Run(out);

	BuildGnuplotScript(getOutputFolder() + Platform::getFilePathSeparator() + getTaskName()+".plot");
}

void GnuplotGraph::BuildGnuplotScript(StringRef file_name)
{
	typedef vector<BenchmarkParameters> 		Results;
	typedef pair<GraphData, Results>			GraphPair;

	vector<GraphPair> graphs;

	for (UInt c = 0; c < tasks_.size(); c++)
	{
		BenchmarkTask* 	task 		= T2T_S<BenchmarkTask*>(tasks_[c]);
		GraphData 		graph_data 	= graph_data_[c];

		Results results;
		String name = task->getFullName();

		for (BenchmarkParameters& result: results_)
		{
			if (result.name() == name)
			{
				results.push_back(result);
			}
		}

		graphs.push_back(GraphPair(graph_data, results));
	}

	for (auto& graph: graphs)
	{
		std::sort(graph.second.begin(), graph.second.end());
	}

	fstream out_file;
	out_file.exceptions ( fstream::failbit | fstream::badbit );
	out_file.open(file_name, fstream::out);

	out_file<<"reset"<<endl;
	out_file<<"set terminal png size "<<this->resolution<<" large"<<endl;
	out_file<<"set output '"+this->getTaskName()+".png'"<<endl;
	out_file<<"set title \""+this->title+"\""<<endl;
	out_file<<"set xlabel \""+this->xtitle+"\""<<endl;
	out_file<<"set ylabel \""+this->ytitle+"\""<<endl;

	out_file<<"set ytics format \""<<ytics_format<<"\""<<(this->y2 ? " nomirror" : "")<<endl;

	if (this->y2)
	{
		out_file<<"set y2label \""+this->y2title+"\""<<endl;
		out_file<<"set y2tics"<<endl;
		out_file<<"set y2tics format \""<<y2tics_format<<"\""<<endl;
	}

	if (this->logscale > 0)
	{
		out_file<<"set logscale x "<<this->logscale<<endl;
	}

	out_file<<"set key "<<this->agenda_location<<endl;

	out_file<<"plot ";

	Int cnt = 0;
	Int size_limit = graphs.size() - 1;
	for (auto& graph: graphs)
	{
		out_file<<"'-' title '"+graph.first.name1<<"' w lp";

		if (cnt < size_limit || (cnt == size_limit && this->y2))
		{
			out_file<<", ";
		}

		if (this->y2)
		{
			out_file<<"'-' title '"+graph.first.name2<<"' axis x1y2 w lp";

			if (cnt < size_limit)
			{
				out_file<<", ";
			}
		}

		cnt++;
	}

	out_file<<endl;

	for (auto& graph: graphs)
	{
		for (auto& result: graph.second)
		{
			BigInt x = result.x() 				/ result.xunit();
			BigInt y1 = result.performance() 	/ result.yunit();

			out_file<<x<<" "<<y1<<endl;
		}

		out_file<<"e"<<endl;

		if (this->y2)
		{
			for (auto& result: graph.second)
			{
				BigInt x = result.x() 				/ result.xunit();
				BigInt y2 = result.throughput() 	/ result.y2unit();

				out_file<<x<<" "<<y2<<endl;
			}

			out_file<<"e"<<endl;
		}
	}

	out_file.close();
}



}
