
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


class TaskParametersSet: public ParametersSet {
public:
	bool 	enabled;
	Int		check_step;
	BigInt	memory_limit;
	bool	own_folder;
public:

	TaskParametersSet(StringRef name):
		ParametersSet(name),
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

	void SetEnabled(bool enabled)
	{
		this->enabled = enabled;
	}

	Int	GetCheckStep() const
	{
		return check_step;
	}
};


struct ExampleTaskParams: public TaskParametersSet {

	Int 	size_;
	Int 	btree_branching_;
	bool 	btree_random_airity_;

	ExampleTaskParams(StringRef name): TaskParametersSet(name), size_(1024), btree_branching_(0), btree_random_airity_(true)
	{
		Add("size", size_);
		Add("btree_branching", btree_branching_);
		Add("btree_random_airity", btree_random_airity_);
	}
};



class Task: public TaskParametersSet {
protected:
	String				constext_name_;
	Int 				iteration_;
	BigInt				duration_;
	String				output_folder_;

	String 				task_name_;

	fstream*			out_;

public:
	Task(StringRef name):
		TaskParametersSet(name),
		iteration_(0),
		duration_(0),
		task_name_(name),
		out_(NULL)
	{}

	virtual ~Task() throw ();

	virtual bool IsGroup() const {
		return false;
	}

	void SetIteration(Int iteration)
	{
		iteration_ = iteration;
	}

	Int GetIteration() const
	{
		return iteration_;
	}

	BigInt GetDuration() const {
		return duration_;
	}

	void SetDuration(BigInt duration) {
		duration_ = duration;
	}

	String GetIterationAsString() const
	{
		return ToString(iteration_);
	}

	StringRef GetOutputFolder() const
	{
		return output_folder_;
	}

	void SetOutputFolder(StringRef folder)
	{
		output_folder_ = folder;
	}

	String GetResourcePath(StringRef name) const
	{
		return output_folder_ + Platform::GetFilePathSeparator() + name;
	}

	bool IsResourceExists(StringRef name) const
	{
		String path = GetResourcePath(name);
		File file(path);
		return file.IsExists();
	}

	template <typename T = TaskParametersSet>
	T* GetParameters() {
		return static_cast<T*>(this);
	}

	template <typename T = TaskParametersSet>
	const T* GetParameters() const {
		return static_cast<const T*>(this);
	}

	virtual String GetTaskName() const
	{
		return task_name_;
	}

	virtual String GetFullName() const
	{
		return GetPrefix();
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

	virtual void SetContextName(StringRef name) {
		constext_name_ = name;
	}

	virtual String GetTaskPropertiesFileName() const {
		return GetTaskName()+".properties";
	}

	virtual String GetTaskParametersFilePath() {
		return GetOutputFolder() + Platform::GetFilePathSeparator() + GetTaskPropertiesFileName();
	}

	static void StoreProperties(const ParametersSet* params, StringRef file_name)
	{
		fstream file;
		file.open(file_name.c_str(), fstream::out | fstream::trunc | fstream::trunc);

		params->DumpProperties(file);

		file.close();
	}

	static void LoadProperties(ParametersSet* params, StringRef file_name)
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

public:

	TaskGroup(StringRef name): Task(name) {
		own_folder = true;
	}

	virtual ~TaskGroup() throw ();

	virtual bool IsGroup() const {
		return true;
	}

	virtual void Run(ostream& out);

	virtual void RegisterTask(Task* task);
	virtual void Configure(Configurator* cfg);

	virtual void BuildResources();
	virtual void ReleaseResources();

	template <typename T>
	T* GetTask(StringRef name)
	{
		for (auto t: tasks_)
		{
			if (t->GetFullName() == name)
			{
				return T2T_S<T*>(t);
			}
			else if (t->IsGroup())
			{
				TaskGroup* group = T2T_S<TaskGroup*>(t);
				T* task = group->GetTask<T>(name);
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

	StringRef GetOutput() const
	{
		return GetOutputFolder();
	}

	virtual String GetTaskOutputFolder(String task_name, Int run) const
	{
		if (IsEmpty(GetOutput()))
		{
			return task_name +"-" + ToString(run);
		}
		else {
			return GetOutput() + Platform::GetFilePathSeparator() + task_name +"-" + ToString(run);
		}
	}

	void SetOutput(StringRef out)
	{
		SetOutputFolder(out);
	}

	Int GetRunCount() const
	{
		return GetParameters<GroupRunner>()->run_count;
	}

	void SetRunCount(Int count)
	{
		GetParameters<GroupRunner>()->run_count = count;
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
