
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <memoria/tools/tests.hpp>

#include <iostream>

namespace memoria {

using namespace std;


void TestRunner::Replay(Configurator* cfg, StringRef dump_file_name)
{
	if (cfg->IsPropertyDefined("task.name"))
	{
		String task_name = cfg->GetProperty("task.name");
		TestTask* task = TestRunner::GetTask<TestTask*>(task_name);
		task->Replay(cfg, dump_file_name);

		cout<<"Replay "<<dump_file_name<<endl;
	}
	else {
		throw MemoriaException(MEMORIA_SOURCE, "Property task.name is not specified");
	}
}



}
