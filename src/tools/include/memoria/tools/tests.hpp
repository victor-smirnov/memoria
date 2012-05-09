
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_TOOLS_TESTS_HPP
#define	_MEMORIA_TOOLS_TESTS_HPP

#include <memoria/tools/task.hpp>

#include <vector>
#include <ostream>
#include <fstream>
#include <limits.h>

namespace memoria {

using namespace std;

class TestReplayParams: public ParametersSet {

	String name_;
	String task_;

	String dump_name_;

	bool replay_;

public:
	TestReplayParams(StringRef name = "Replay", StringRef task = "", StringRef prefix = ""):ParametersSet(prefix), name_(name), task_(task), replay_(false)
	{
		Add("name", name_);
		Add("task", task_);
		Add("dump_name", dump_name_);
	}

	virtual ~TestReplayParams() {}

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

protected:
	Int 	size_;
	Int 	btree_branching_;
	bool 	btree_random_branching_;

public:
	TestTask(StringRef name):
		Task(name),
		size_(200),
		btree_branching_(0),
		btree_random_branching_(true)
	{
		own_folder = true;

		Add("size", size_);
		Add("btree_branching", btree_branching_);
		Add("btree_random_branching", btree_random_branching_);
	}

	virtual ~TestTask() throw () 			{}

	virtual TestReplayParams* ReadTestStep(Configurator* cfg) const;

	virtual void 			Replay(ostream& out, Configurator* cfg);
	virtual void 			Configure(TestReplayParams* params) const;


	virtual TestReplayParams* CreateTestStep(StringRef name) const						= 0;
	virtual void 			Run(ostream& out)											= 0;
	virtual void 			Replay(ostream& out, TestReplayParams* step_params)			= 0;


	virtual void Store(TestReplayParams* params) const
	{
		Configure(params);

		String props_name = GetPropertiesFileName();
		StoreProperties(params, props_name);
	}

	virtual String GetPropertiesFileName(StringRef infix = "") const
	{
		return GetResourcePath("Replay"+infix+".properties");
	}

	virtual String GetParametersFilePath() {
		return GetResourcePath("Task");
	}

	virtual String GetTaskPropertiesFileName() const {
		return "ReplayTask.properties";
	}

	String GetFileName(StringRef name) const;

};



class TestSuite: public TaskGroup {
public:
	TestSuite(StringRef name): TaskGroup(name)
	{
	}

	virtual ~TestSuite() throw() {}
};



class MemoriaTestRunner: public MemoriaTaskRunner {
public:
	MemoriaTestRunner(): MemoriaTaskRunner() 		{}
	virtual ~MemoriaTestRunner() throw ()			{}

	void Replay(ostream& out, StringRef replay_file);

	virtual Int Run();
};




}
#endif
