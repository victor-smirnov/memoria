
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
	TaskParametersSet*	parameters_;
	bool 				enabled_;

public:
	Task(TaskParametersSet* parameters): parameters_(parameters) {}

	virtual ~Task() throw ();

	TaskParametersSet* GetParameters() {
		return parameters_;
	}

	bool IsRunByDefault() const
	{
		return parameters_->IsEnabled();
	}

	virtual void Run(ostream& out) = 0;
};

class TaskRunner {

	vector<Task*> 	tasks_;

public:
	TaskRunner() {}

	~TaskRunner();

	void RegisterTask(Task* task) {
		tasks_.push_back(task);
	}

	void Configure(Configurator* cfg);

	void DumpProperties(ostream& os);
	void Run(ostream& out);
};

}
#endif
