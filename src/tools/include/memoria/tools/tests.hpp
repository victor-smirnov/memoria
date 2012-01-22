
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_TOOLS_TESTS_HPP
#define	_MEMORIA_TOOLS_TESTS_HPP


#include <memoria/tools/task.hpp>
#include <memoria/tools/tools.hpp>
#include <memoria/memoria.hpp>

#include <map>
#include <memory>
#include <fstream>

namespace memoria {

using namespace std;

class TestStepParams: public ParametersSet {

	String name_;
	String task_;

	String dump_name_;

	bool replay_;

public:
	TestStepParams(StringRef name = "", StringRef task = "", StringRef prefix = ""):ParametersSet(prefix), name_(name), task_(task), replay_(false)
	{
		Add("name", name_);
		Add("task", task_);
		Add("dumpName", dump_name_);
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

	StringRef GetDumpName() const
	{
		return dump_name_;
	}

	void SetDumpName(String file_name)
	{
		this->dump_name_ = file_name;
	}

	bool IsReplay() const
	{
		return replay_;
	}

	void SetReplay(bool replay)
	{
		replay_ = replay;
	}
};

class TestTask: public Task {

public:
	TestTask(TaskParametersSet* parameters): Task(parameters) {}
	virtual ~TestTask() throw () {}

	virtual TestStepParams* ReadTestStep(Configurator* cfg) const;

	virtual void 			Replay(ostream& out, Configurator* cfg);
	virtual void 			Configure(TestStepParams* params) const;


	virtual TestStepParams* CreateTestStep(StringRef name) const						= 0;
	virtual void 			Run(ostream& out)											= 0;
	virtual void 			Replay(ostream& out, TestStepParams* step_params)			= 0;

public:

	String GetFileName(StringRef name) const;
};


template <typename Profile_, typename Allocator_>
class ProfileTestTask: public TestTask {

public:

	typedef Profile_ 								Profile;
	typedef Allocator_ 								Allocator;


	ProfileTestTask(TaskParametersSet* parameters): TestTask(parameters) {}
	virtual ~ProfileTestTask() throw () {};

	virtual TestStepParams* CreateTestStep(StringRef name) const						= 0;
	virtual void 			Run(ostream& out)											= 0;
	virtual void 			Replay(ostream& out, TestStepParams* step_params)			= 0;

	virtual void LoadAllocator(Allocator& allocator, StringRef file_name) const
	{
		unique_ptr <FileInputStreamHandler> in(FileInputStreamHandler::create(file_name.c_str()));
		allocator.load(in.get());
	}

	virtual void LoadAllocator(Allocator& allocator, const TestStepParams* params) const
	{
		unique_ptr <FileInputStreamHandler> in(FileInputStreamHandler::create(params->GetDumpName().c_str()));
		allocator.load(in.get());
	}

	virtual void StoreAllocator(Allocator& allocator, StringRef file_name) const
	{
		unique_ptr <FileOutputStreamHandler> out(FileOutputStreamHandler::create(file_name.c_str()));
		allocator.store(out.get());
	}

	virtual void StoreProperties(const TestStepParams* params, StringRef file_name) const
	{
		fstream file;
		file.open(file_name.c_str(), fstream::out | fstream::trunc | fstream::trunc);

		params->DumpProperties(file);

		file.close();
	}

	virtual void Store(Allocator& allocator, TestStepParams* params) const
	{
		Configure(params);

		String file_name = GetAllocatorFileName(params, ".valid");
		StoreAllocator(allocator, file_name);
		params->SetDumpName(file_name);

		String file_name_invalid = GetAllocatorFileName(params, ".invalid");
		allocator.commit();
		StoreAllocator(allocator, file_name_invalid);

		String props_name = GetPropertiesFileName(params);
		StoreProperties(params, props_name);
	}

	virtual String GetAllocatorFileName(const TestStepParams* params, StringRef infix = "") const
	{
		return GetTaskName()+"."+params->GetName()+infix+".dump";
	}

	virtual String GetPropertiesFileName(const TestStepParams* params, StringRef infix = "") const
	{
		return GetTaskName()+"."+params->GetName()+infix+".properties";
	}
};


class SPTestTask: public ProfileTestTask<StreamProfile<>, DefaultStreamAllocator> {

	typedef ProfileTestTask<StreamProfile<>, DefaultStreamAllocator> Base;

public:
	SPTestTask(TaskParametersSet* parameters): Base(parameters) {}
	virtual ~SPTestTask() throw () {};

	virtual TestStepParams* CreateTestStep(StringRef name) const						= 0;
	virtual void 			Run(ostream& out)											= 0;
	virtual void 			Replay(ostream& out, TestStepParams* step_params)			= 0;

	void Check(Allocator& allocator, const char* source)
	{
		::memoria::Check<Allocator, ::memoria::StreamContainersChecker>(allocator, "Allocator check failed", source);
	}

	void Check(Allocator& allocator, const char* message, const char* source)
	{
		::memoria::Check<Allocator, ::memoria::StreamContainersChecker>(allocator, message, source);
	}
};



class TestRunner: public TaskRunner {
public:
	TestRunner(): TaskRunner() 	{}
	virtual ~TestRunner() 		{}
};


}
#endif
