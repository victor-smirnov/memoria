
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_TOOLS_TASK_HPP
#define	_MEMORIA_TOOLS_TASK_HPP

#include <memoria/core/types/types.hpp>

#include <vector>

namespace memoria {

class TaskParameter;

using namespace std;

class Task {
	String 	name_;
	bool 	run_by_default_;
	vector<TaskParameter*> parameters_;
public:
	Task(StringRef name, bool run_by_default): name_(name), run_by_default_(run_by_default) {}

	virtual ~Task() throw ();

	virtual void run() = 0;
};

}
#endif
