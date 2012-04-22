
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_TOOLS_BENCHMARKS_HPP
#define	_MEMORIA_TOOLS_BENCHMARKS_HPP


#include <memoria/tools/task.hpp>
#include <memoria/tools/tools.hpp>
#include <memoria/memoria.hpp>

#include <map>
#include <memory>
#include <fstream>
#include <vector>

namespace memoria {

using namespace std;

class BenchmarkResult {

	String name_;
	BigInt x_;
	BigInt operations_;
	BigInt time_;
	BigInt runs_;

	double value_;

public:
	BenchmarkResult(String name): name_(name), x_(0), operations_(0), time_(0), runs_(1) {}

	StringRef name() const {
		return name_;
	}

	const BigInt& x() const {
		return x_;
	}

	BigInt& x() {
		return x_;
	}

	const BigInt& runs() const {
		return runs_;
	}

	BigInt& runs() {
		return runs_;
	}

	const double& value() const {
		return value_;
	}

	double& value() {
		return value_;
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

	void operator+=(const BenchmarkResult& other)
	{
		this->operations_ 	+= other.operations();
	}


	bool operator<(const BenchmarkResult& other) const
	{
		return x_ < other.x_;
	}

	double speed() const
	{
		return this->operations_/runs_/(double)time_;
	}

	double plot_value() const
	{
		return value_ / runs_;
	}
};

class BenchmarkTask;

class BenchmarkGroup: public TaskParametersSet {
public:
	typedef vector<BenchmarkResult> Results;
	typedef vector<BenchmarkTask*>  Tasks;
private:

	Results results_;
	Tasks	tasks_;
	String 	output_folder_;
	BigInt 	duration_;
	String 	title_;
	String 	xtitle_;
	String 	ytitle_;

	String 	resolution_;
	String  agenda_location_;

	bool time_;
	Int logscale_;

public:
	BenchmarkGroup(StringRef name, StringRef title = "Benchmark", StringRef xtitle = "X", StringRef ytitle = "Y", Int logscale = 2, bool time = true):
		TaskParametersSet(name),
		duration_(0),
		title_(title),
		xtitle_(xtitle),
		ytitle_(ytitle),
		time_(time)
	{
		Add("time", time_, time);
		Add("resolution", resolution_, String("800,600"));
		Add("agenda_location", agenda_location_, String("top left"));
		Add("logscale", logscale_, logscale);
	}

	StringRef title() const {
		return title_;
	}

	StringRef xtitle() const {
		return xtitle_;
	}

	StringRef ytitle() const {
		return ytitle_;
	}

	StringRef name() const {
		return GetPrefix();
	}

	const Results& results() const {
		return results_;
	}

	Results& results() {
		return results_;
	}

	const Tasks& tasks() const {
		return tasks_;
	}

	Tasks& tasks() {
		return tasks_;
	}

	StringRef output_folder() const {
		return output_folder_;
	}

	String& output_folder() {
		return output_folder_;
	}

	BigInt& duration() {
		return duration_;
	}

	const BigInt& duration() const {
		return duration_;
	}

	bool time() const {
		return time_;
	}

	StringRef resolution() const {
		return resolution_;
	}

	String& resolution() {
		return resolution_;
	}

	StringRef& agenda_location() const {
		return agenda_location_;
	}

	String& agenda_location() {
		return agenda_location_;
	}

	Int& logscale() {
		return logscale_;
	}

	const Int& logscale() const {
		return logscale_;
	}
};


struct BenchmarkParams: public TaskParametersSet {

	Int count_;
	Int times_;

	BenchmarkParams(StringRef name): TaskParametersSet(name)
	{
		Add("count", count_, 1);
		Add("times", times_, 1);
	}
};


class BenchmarkTask: public Task {

	BenchmarkGroup* group_;

public:
	BenchmarkTask(BenchmarkParams* parameters): Task(parameters)
	{}

	virtual ~BenchmarkTask() throw () {}

	void SetGroup(BenchmarkGroup* group)
	{
		group_ = group;
		TaskParametersSet* parameters = GetParameters();

		parameters->SetPrefix(group_->name()+"."+parameters->GetPrefix());
	}

	virtual void Run(ostream& out);

	virtual void Benchmark(BenchmarkResult& result, ostream& out) 			= 0;

	virtual String GetGraphName() {
		return GetTaskName();
	}

	virtual Int times() const
	{
		return GetParameters<BenchmarkParams>()->times_;
	}

public:

	String GetFileName(StringRef name) const;
};


template <typename Profile_, typename Allocator_>
class ProfileBenchmarkTask: public BenchmarkTask {

public:

	typedef Profile_ 								Profile;
	typedef Allocator_ 								Allocator;


	ProfileBenchmarkTask(BenchmarkParams* parameters): BenchmarkTask(parameters) {}
	virtual ~ProfileBenchmarkTask() throw () {};




	virtual void LoadAllocator(Allocator& allocator, StringRef file_name) const
	{
		unique_ptr <FileInputStreamHandler> in(FileInputStreamHandler::create(file_name.c_str()));
		allocator.load(in.get());
	}

	virtual void StoreAllocator(Allocator& allocator, StringRef file_name) const
	{
		unique_ptr <FileOutputStreamHandler> out(FileOutputStreamHandler::create(file_name.c_str()));
		allocator.store(out.get());
	}

};


class SPBenchmarkTask: public ProfileBenchmarkTask<SmallProfile<>, SmallInMemAllocator> {

	typedef ProfileBenchmarkTask<SmallProfile<>, SmallInMemAllocator> Base;

public:
	SPBenchmarkTask(BenchmarkParams* parameters): Base(parameters) {}
	virtual ~SPBenchmarkTask() throw () {};

	void Check(Allocator& allocator, const char* source)
	{
		::memoria::Check<Allocator>(allocator, "Allocator check failed", source);
	}

	void Check(Allocator& allocator, const char* message, const char* source)
	{
		::memoria::Check<Allocator>(allocator, message, source);
	}

	template <typename CtrType>
	void CheckCtr(CtrType& ctr, const char* message, const char* source)
	{
		::memoria::CheckCtr<CtrType>(ctr, message, source);
	}

	template <typename CtrType>
	void CheckCtr(CtrType& ctr, const char* source)
	{
		CheckCtr(ctr, "Container check failed", source);
	}
};


class BenchmarkRunner: public TaskRunner {
public:
	typedef vector<BenchmarkGroup*> Groups;
private:

	Groups groups_;

public:
	BenchmarkRunner() 			{}
	virtual ~BenchmarkRunner() 	{}

	virtual void Configure(Configurator* cfg);

	BenchmarkGroup* BeginGroup(BenchmarkGroup* group)
	{
		groups_.push_back(group);
		return group;
	}

	void EndGroup() {}

	virtual void RegisterBenchmark(BenchmarkTask* task)
	{
		BenchmarkGroup* group = groups_[groups_.size() - 1];
		task->SetGroup(group);

		group->tasks().push_back(task);

		this->RegisterTask(task);
	}

	virtual Int Run(ostream& out);

protected:
	void BuildGnuplotScript(BenchmarkGroup* group, StringRef file_name);
};




}
#endif
