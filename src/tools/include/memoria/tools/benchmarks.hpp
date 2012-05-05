
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_TOOLS_BENCHMARKS_HPP
#define	_MEMORIA_TOOLS_BENCHMARKS_HPP


#include <memoria/tools/task.hpp>
#include <memoria/tools/tools.hpp>


#include <map>
#include <memory>
#include <fstream>
#include <vector>

namespace memoria {

using namespace std;


class BenchmarkParameters {

	String name_;
	BigInt x_;
	BigInt xunit_;
	BigInt yunit_;
	BigInt operations_;
	BigInt time_;
	BigInt runs_;

public:
	BenchmarkParameters(String name): name_(name), x_(0), xunit_(1), yunit_(1), operations_(0), time_(0), runs_(1) {}


	StringRef name() const {
		return name_;
	}

	const BigInt& x() const {
		return x_;
	}

	BigInt& x() {
		return x_;
	}

	const BigInt& xunit() const {
		return xunit_;
	}

	BigInt& yunit() {
		return yunit_;
	}

	const BigInt& yunit() const {
		return yunit_;
	}

	BigInt& xunit() {
		return xunit_;
	}

	const BigInt& runs() const {
		return runs_;
	}

	BigInt& runs() {
		return runs_;
	}

	const BigInt& operations() const {
		return operations_;
	}

	BigInt& operations() {
		return operations_;
	}

	const BigInt& time() const {
		return time_;
	}

	BigInt& time() {
		return time_;
	}

	bool operator<(const BenchmarkParameters& other) const
	{
		return x_ < other.x_;
	}

	BigInt plot_value() const
	{
		return operations_ * 1000 / time_ ;
	}
};


class BenchmarkTask: public Task {

public:

	Int average;

	String graph_name_;

public:
	BenchmarkTask(StringRef name, StringRef graph_name): Task(name), average(1), graph_name_(name)
	{
		Add("average", average);
		Add("graph_name", graph_name_);
	}

	virtual ~BenchmarkTask() throw () {}

	virtual Int GetAverage() {
		return average;
	}

	virtual void Run(ostream& out) {}

	virtual void Prepare(BenchmarkParameters& result)
	{
		Prepare(result, *out_);
	}

	virtual void Release()
	{
		Release(*out_);
	}

	virtual void Benchmark(BenchmarkParameters& result)
	{
		Benchmark(result, *out_);
	}

	virtual void Prepare(BenchmarkParameters& result, ostream& out) {}

	virtual void BeforeBenchmark(BenchmarkParameters& result, ostream& out) {}
	virtual void Benchmark(BenchmarkParameters& result, ostream& out) 		= 0;
	virtual void AfterBenchmark(BenchmarkParameters& result, ostream& out)  {}

	virtual void Release(ostream& out) {}

	virtual StringRef GetGraphName() {
		return graph_name_;
	}

public:

	String GetFileName(StringRef name) const;
};



class BenchmarkTaskGroup: public TaskGroup {

protected:

	typedef vector<BenchmarkParameters> Results;
	Results results_;

public:

	BigInt time_start;
	BigInt time_stop;

	BigInt current_time;
	BigInt operations;

	BigInt xunit;
	BigInt yunit;

	BenchmarkTaskGroup(StringRef name):
		TaskGroup(name),
		time_start(0),
		operations(1024*1024),
		xunit(1),
		yunit(1)
	{
		Add("time_start", time_start);
		Add("time_stop",  time_stop);
		Add("operations", operations);
		Add("xunit", 	  xunit);
		Add("yunit", 	  yunit);
	}

	virtual ~BenchmarkTaskGroup() throw ();

	virtual BigInt NextTime() {
		return current_time++;
	}

	virtual void ResetTime() {
		current_time = time_start;
	}

	virtual bool IsEnd() {
		return current_time > time_stop;
	}

	virtual void Run(ostream& out);

	virtual void RegisterTask(BenchmarkTask* task);
};


class GnuplotGraph: public BenchmarkTaskGroup
{
public:
	String 	title;
	String 	xtitle;
	String 	ytitle;

	String 	resolution;
	String  agenda_location;

	bool 	time;
	Int 	logscale;



	GnuplotGraph(String name):
		BenchmarkTaskGroup(name),
		title("Graph"),
		xtitle("X Axis"),
		ytitle("Y Axis"),
		resolution("800,600"),
		agenda_location("top left"),
		time(false),
		logscale(2)
	{
		Add("title", title);
		Add("xtitle", xtitle);
		Add("ytitle", ytitle);
		Add("resolution", resolution);
		Add("agenda_location", agenda_location);
		Add("logscale", logscale);
	}

	virtual ~GnuplotGraph() throw () {}

	virtual void Run(ostream& out);

protected:
	void BuildGnuplotScript(StringRef file_name);
};


//class BenchmarkRunner: public TaskRunner {
//public:
//	typedef vector<BenchmarkGroup*> Groups;
//private:
//
//	Groups groups_;
//
//public:
//	BenchmarkRunner() 			{}
//	virtual ~BenchmarkRunner() 	{}
//
//	virtual void Configure(Configurator* cfg);
//
//	BenchmarkGroup* BeginGroup(BenchmarkGroup* group)
//	{
//		groups_.push_back(group);
//		return group;
//	}
//
//	void EndGroup() {}
//
//
//	virtual Int Run(ostream& out);
//
//protected:
//	void BuildGnuplotScript(BenchmarkGroup* group, StringRef file_name);
//};


}
#endif
