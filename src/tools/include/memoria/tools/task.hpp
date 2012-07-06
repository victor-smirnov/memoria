
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_TOOLS_TASK_HPP
#define	_MEMORIA_TOOLS_TASK_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/tools/params.hpp>
#include <memoria/core/tools/file.hpp>

#include <vector>
#include <ostream>
#include <fstream>
#include <limits.h>

namespace memoria {

using namespace std;


class TaskParametersset: public Parametersset {
public:
	bool 	enabled;
	Int		check_step;
	BigInt	memory_limit;
	bool	own_folder;
public:

	TaskParametersset(StringRef name):
		Parametersset(name),
		enabled(true),
		check_step(1),
		memory_limit(LLONG_MAX),
		own_folder(false)
	{
		Add("enabled", enabled);
		Add("check_step", check_step);
		Add("memory_limit", memory_limit);
		Add("own_folder", own_folder);
	}

	bool IsEnabled() const
	{
		return enabled;
	}

	void setEnabled(bool enabled)
	{
		this->enabled = enabled;
	}

	Int	getcheckStep() const
	{
		return check_step;
	}
};


struct ExampleTaskParams: public TaskParametersset {

	Int 	size_;
	Int 	btree_branching_;
	bool 	btree_random_airity_;

	ExampleTaskParams(StringRef name): TaskParametersset(name), size_(1024), btree_branching_(0), btree_random_airity_(true)
	{
		Add("size", size_);
		Add("btree_branching", btree_branching_);
		Add("btree_random_airity", btree_random_airity_);
	}
};



class Task: public TaskParametersset {
protected:
	String				constext_name_;
	Int 				iteration_;
	BigInt				duration_;
	String				output_folder_;

	String 				task_name_;

	fstream*			out_;

public:
	Task(StringRef name):
		TaskParametersset(name),
		iteration_(0),
		duration_(0),
		task_name_(name),
		out_(NULL)
	{}

	virtual ~Task() throw ();

	virtual bool IsGroup() const {
		return false;
	}

	void setIteration(Int iteration)
	{
		iteration_ = iteration;
	}

	Int getIteration() const
	{
		return iteration_;
	}

	BigInt getDuration() const {
		return duration_;
	}

	void setDuration(BigInt duration) {
		duration_ = duration;
	}

	String getIterationAsString() const
	{
		return ToString(iteration_);
	}

	StringRef getOutputFolder() const
	{
		return output_folder_;
	}

	void setOutputFolder(StringRef folder)
	{
		output_folder_ = folder;
	}

	String getResourcePath(StringRef name) const
	{
		return output_folder_ + Platform::getFilePathSeparator() + name;
	}

	bool IsResourceExists(StringRef name) const
	{
		String path = getResourcePath(name);
		File file(path);
		return file.IsExists();
	}

	template <typename T = TaskParametersset>
	T* getParameters() {
		return static_cast<T*>(this);
	}

	template <typename T = TaskParametersset>
	const T* getParameters() const {
		return static_cast<const T*>(this);
	}

	virtual String getTaskName() const
	{
		return task_name_;
	}

	virtual String getFullName() const
	{
		return getPrefix();
	}

	virtual void BuildResources();
	virtual void ReleaseResources();

	virtual void Prepare() {
		Prepare(*out_);
	}

	virtual Int Run();

	virtual void Release() {
		Release(*out_);
	}

	virtual void Prepare(ostream& out) {}
	virtual void Run(ostream& out) 				= 0;
	virtual void Release(ostream& out) {}


	virtual void Configure(Configurator* cfg);

	virtual void setContextName(StringRef name) {
		constext_name_ = name;
	}

	virtual String getTaskPropertiesFileName() const {
		return getTaskName()+".properties";
	}

	virtual String getTaskParametersFilePath() {
		return getOutputFolder() + Platform::getFilePathSeparator() + getTaskPropertiesFileName();
	}

	static void StoreProperties(const Parametersset* params, StringRef file_name)
	{
		fstream file;
		file.open(file_name.c_str(), fstream::out | fstream::trunc | fstream::trunc);

		params->DumpProperties(file);

		file.close();
	}

	static void LoadProperties(Parametersset* params, StringRef file_name)
	{
		fstream file;
		file.open(file_name.c_str(), fstream::in | fstream::trunc | fstream::trunc);

		Configurator cfg;
		Configurator::Parse(file_name.c_str(), &cfg);

		params->Process(&cfg);

		file.close();
	}
};


class TaskGroup: public Task {
public:
	typedef vector<Task*>  Tasks;

protected:
	Tasks	tasks_;

	struct FailureDescriptor {
		Int run_number;
		String task_name;

		FailureDescriptor()	{}
		FailureDescriptor(Int number, StringRef name): run_number(number), task_name(name) {}
	};

	vector<FailureDescriptor> failures_;

public:

	TaskGroup(StringRef name): Task(name) {
		own_folder = true;
	}

	virtual ~TaskGroup() throw ();

	virtual bool IsGroup() const {
		return true;
	}

	virtual void Run(ostream& out);

	virtual void registerTask(Task* task);
	virtual void Configure(Configurator* cfg);

	virtual void BuildResources();
	virtual void ReleaseResources();

	virtual void OnFailure(Task* task) {}

	virtual Int Run()
	{
		Task::Run();

		return failures_.size();
	}

	template <typename T>
	T* getTask(StringRef name)
	{
		for (auto t: tasks_)
		{
			if (t->getFullName() == name)
			{
				return T2T_S<T*>(t);
			}
			else if (t->IsGroup())
			{
				TaskGroup* group = T2T_S<TaskGroup*>(t);
				T* task = group->getTask<T>(name);
				if (task != NULL)
				{
					return task;
				}
			}
		}

		return NULL;
	}
};


class GroupRunner: public TaskGroup {
public:

	Int run_count;

	GroupRunner(StringRef name): TaskGroup(name), run_count(1)
	{
		Add("run_count", run_count);
	}

	virtual ~GroupRunner() throw() {}

	StringRef getOutput() const
	{
		return getOutputFolder();
	}

	virtual String getTaskOutputFolder(String task_name, Int run) const
	{
		if (IsEmpty(getOutput()))
		{
			return task_name +"-" + ToString(run);
		}
		else {
			return getOutput() + Platform::getFilePathSeparator() + task_name +"-" + ToString(run);
		}
	}

	void setOutput(StringRef out)
	{
		setOutputFolder(out);
	}

	Int getRunCount() const
	{
		return getParameters<GroupRunner>()->run_count;
	}

	void setRunCount(Int count)
	{
		getParameters<GroupRunner>()->run_count = count;
	}


	virtual void DumpProperties(ostream& out) {}

	virtual Int Run();
};


class MemoriaTaskRunner: public GroupRunner {
public:
	MemoriaTaskRunner(): GroupRunner("") 			{}
	virtual ~MemoriaTaskRunner() throw ()			{}
};



}
#endif
