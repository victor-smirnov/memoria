
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_TOOLS_TESTS_HPP
#define	_MEMORIA_TOOLS_TESTS_HPP


#include <memoria/tools/task.hpp>
#include <memoria/memoria.hpp>

#include <map>
#include <memory>

namespace memoria {

using namespace std;

class TestStepParams: public ParametersSet {

	String name_;
	String task_;

public:
	TestStepParams():ParametersSet("")
	{
		Add("name", name_);
		Add("task", task_);
	}

	virtual ~TestStepParams() {}

	StringRef GetName() const
	{
		return name_;
	}

	void SetName(StringRef name)
	{
		name_ = name;
	}

	StringRef GetTask() const
	{
		return task_;
	}

	void SetTask(StringRef task)
	{
		task_ = task;
	}
};

class TestTask: public Task {

public:
	TestTask(TaskParametersSet* parameters): Task(parameters) {}
	virtual ~TestTask() throw () {}

	virtual TestStepParams* ReadTestStep(Configurator* cfg);
	virtual void 			Run(ostream& out, Configurator* cfg);

	virtual TestStepParams* CreateTestStep(StringRef name)								= 0;
	virtual void 			Run(ostream& out, TestStepParams* step_params)				= 0;

public:

	String GetFileName(StringRef name);
};


template <typename Profile_, typename Allocator_>
class ProfileTestTask: public TestTask {

public:

	typedef Profile_ 								Profile;
	typedef Allocator_ 								Allocator;


	ProfileTestTask(TaskParametersSet* parameters): TestTask(parameters) {}
	virtual ~ProfileTestTask() throw () {};

	virtual TestStepParams* CreateTestStep(StringRef name)								= 0;
	virtual void 			Run(ostream& out, TestStepParams* step_params)				= 0;

	void LoadAllocator(Allocator& allocator, StringRef file_name)
	{
		unique_ptr <FileInputStreamHandler> in(FileInputStreamHandler::create(file_name.c_str()));
		allocator.load(in.get());
	}

	void StoreAllocator(Allocator& allocator, StringRef file_name)
	{
		unique_ptr <FileInputStreamHandler> out(FileInputStreamHandler::create(file_name.c_str()));
		allocator.load(out.get());
	}

	String GetAllocatorFileName(const TestStepParams* params) const
	{
		return GetTaskName()+"."+params->GetName()+".dump";
	}
};


class SPTestTask: public ProfileTestTask<StreamProfile<>, DefaultStreamAllocator> {

	typedef ProfileTestTask<StreamProfile<>, DefaultStreamAllocator> Base;

public:
	SPTestTask(TaskParametersSet* parameters): Base(parameters) {}
	virtual ~SPTestTask() throw () {};

	virtual TestStepParams* CreateTestStep(StringRef name)								= 0;
	virtual void 			Run(ostream& out, TestStepParams* step_params)				= 0;
};



class TestRunner: public TaskRunner {
public:
	TestRunner(): TaskRunner() 	{}
	virtual ~TestRunner() 		{}
};


}
#endif
