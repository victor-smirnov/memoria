
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <memoria/tools/tests.hpp>

#include <iostream>

namespace memoria {

using namespace std;
using namespace memoria::vapi;

void TestRunner::Replay(Configurator* cfg, StringRef dump_file_name)
{
	if (cfg->IsPropertyDefined("task.name"))
	{
		String task_name = cfg->GetProperty("task.name");
		TestTask* task = TestRunner::GetTask<TestTask*>(task_name);
		cout<<"Replay "<<dump_file_name<<endl;
		task->Replay(cfg, dump_file_name);
	}
	else {
		throw MemoriaException(MEMORIA_SOURCE, "Property task.name is not specified");
	}
}


TestTask::~TestTask() throw ()
{
	for (auto i = operations_.begin(); i != operations_.end(); i++)
	{
		delete i->second;
	}
}


void TestTask::RegisterUpdateOp(UpdateOp* op)
{
	operations_[op->GetName()] = op;
}


UpdateOp* TestTask::GetUpdateOp(StringRef name) const
{
	auto i = operations_.find(name);
	if (i != operations_.end())
	{
		return i->second;
	}
	else {
		throw MemoriaException(MEMORIA_SOURCE, "Unknown update operation "+name+" for task "+GetTaskName());
	}
}

void TestTask::Replay(Configurator* cfg, StringRef dump_file_name)
{
	if (cfg->IsPropertyDefined("op.name"))
	{
		String op_name = cfg->GetProperty("op.name");
		UpdateOp* op = GetUpdateOp(op_name);
		op->Configure(cfg);
		ExecuteUpdateOp(op, dump_file_name);
	}
	else {
		throw MemoriaException(MEMORIA_SOURCE, "Property op.name is not specified");
	}
}

void TestTask::Run(ostream& out)
{
	Prepare();
	try {
		ExecuteTask(out);
	}
	catch (...) {
		DumpAllocator();
		Finish();
	}
	Finish();
}


}
