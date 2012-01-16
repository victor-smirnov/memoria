
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

class Task {
	ParametersSet*	parameters_;

public:
	Task(ParametersSet* parameters): parameters_(parameters) {}

	virtual ~Task() throw ();

	ParametersSet* GetParameters() {
		return parameters_;
	}

	const ParametersSet* GetParameters() const {
		return parameters_;
	}

	StringRef GetTaskName() const
	{
		return parameters_->GetName();
	}

	bool IsRunByDefault() const
	{
		return parameters_->IsEnabled();
	}

	virtual void Run(ostream& out) = 0;
};




class TaskRunner {

	map<String, Task*> 	tasks_;

public:
	TaskRunner() {}

	~TaskRunner();

	void RegisterTask(Task* task) {
		tasks_[task->GetParameters()->GetName()] = task;
	}

	void Configure(Configurator* cfg);

	void DumpProperties(ostream& os);
	void Run(ostream& out);

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
