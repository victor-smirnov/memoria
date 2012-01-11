
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_TOOLS_TESTS_HPP
#define	_MEMORIA_TOOLS_TESTS_HPP


#include <memoria/tools/task.hpp>
#include <memoria/memoria.hpp>


namespace memoria {

using namespace std;

class TestTask: public Task {
public:
	TestTask(TaskParametersSet* parameters): Task(parameters) {}
	virtual ~TestTask() throw () {};

	virtual void Replay(Configurator* cfg, StringRef dump_file_name) = 0;
};


template <typename Profile, typename Allocator>
class ProfileTestTask: public TestTask {



public:
	ProfileTestTask(TaskParametersSet* parameters): TestTask(parameters) {}
	virtual ~ProfileTestTask() throw () {};

	virtual void Replay(Configurator* cfg, StringRef dump_file_name)
	{

	}
};


class TestRunner: public TaskRunner {
public:
	TestRunner(): TaskRunner() {}

	void Replay(Configurator* cfg, StringRef dump_file_name);
};


}
#endif
