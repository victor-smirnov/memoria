
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_TOOLS_TASK_HPP
#define	_MEMORIA_TOOLS_TASK_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/tools/params.hpp>

#include <vector>
#include <ostream>

namespace memoria {

using namespace std;


class TaskParametersSet: public ParametersSet {
	bool enabled_;
public:

	TaskParametersSet(StringRef name): ParametersSet(name), enabled_(true)
	{
		Add(true, "enabled", enabled_);
	}

	bool IsEnabled() const
	{
		return enabled_;
	}

	void SetEnabled(bool enabled)
	{
		enabled_ = enabled;
	}
};


class Task {
	TaskParametersSet*	parameters_;
	Int 				iteration_;
	BigInt				duration_;
	String				output_folder_;

public:
	Task(TaskParametersSet* parameters): parameters_(parameters), iteration_(0), duration_(0) {}

	virtual ~Task() throw ();

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

	template <typename T = TaskParametersSet>
	T* GetParameters() {
		return static_cast<T*>(parameters_);
	}

	template <typename T = TaskParametersSet>
	const T* GetParameters() const {
		return static_cast<const T*>(parameters_);
	}

	StringRef GetTaskName() const
	{
		return parameters_->GetPrefix();
	}

	bool IsRunByDefault() const
	{
		return parameters_->IsEnabled();
	}

	virtual void Run(ostream& out) 							= 0;
	virtual void Replay(ostream& out, Configurator* cfg) 	= 0;
};




class TaskRunner {

	map<String, Task*> 	tasks_;
	Int 				run_count_;
	String				output_;

public:
	TaskRunner(): run_count_(1) {}

	~TaskRunner();

	void RegisterTask(Task* task)
	{
		tasks_[task->GetParameters<>()->GetPrefix()] = task;
	}

	void Configure(Configurator* cfg);
	void DumpProperties(ostream& os);

	void Run(ostream& out);
	void Replay(ostream& out, StringRef replay_file);

	StringRef GetOutput() const
	{
		return output_;
	}

	virtual String GetTaskOutputFolder(String task_name, Int run) const
	{
		if (IsEmpty(output_))
		{
			return task_name +"-" + ToString(run);
		}
		else {
			return output_ + Platform::GetFilePathSeparator() + task_name +"-" + ToString(run);
		}
	}

	void SetOutput(StringRef out)
	{
		output_ = out;
	}

	Int GetRunCount() const
	{
		return run_count_;
	}

	void SetRunCount(Int count)
	{
		run_count_ = count;
	}

	template <typename T>
	T GetTask(StringRef name)
	{
		auto i = tasks_.find(name);
		if (i != tasks_.end())
		{
			return static_cast<T>(i->second);
		}
		else {
			throw MemoriaException(MEMORIA_SOURCE, "Task "+name+" is not registered");
		}
	}
};

}
#endif
