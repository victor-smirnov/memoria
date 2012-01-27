
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include "kv_map/task.hpp"
#include "sum_set/task.hpp"
#include "vector/task.hpp"
#include "template/task.hpp"

#include <memoria/tools/cmdline.hpp>

#include <iostream>

using namespace std;
using namespace memoria;

MEMORIA_INIT();

const char* DESCRIPTION = "Run Memoria regression tests with specified configuration";
const char* CFG_FILE	= "tests.properties";

int main(int argc, const char** argv, const char** envp)
{
	try {
		CmdLine cmd_line(argc, argv, envp, CFG_FILE, CmdLine::REPLAY);

		//FIXME: C++11 RNG seed doesn't work
		Seed(GetTimeInMillis());
		SeedBI(GetTimeInMillis());

		TestRunner runner;

		// add tasks to the runner;

		runner.RegisterTask(new KVMapTestTask());
		runner.RegisterTask(new SumSetTestTask());
		runner.RegisterTask(new VectorTestTask());
		runner.RegisterTask(new TemplateTestTask());

		runner.Configure(&cmd_line.GetConfigurator());

		if (cmd_line.IsHelp())
		{
			cout<<endl;
			cout<<"Description: "<<DESCRIPTION<<endl;
			cout<<"Usage: "<<cmd_line.GetImageName()<<" [--help] [--dump] [--config <file.properties>]"<<endl;
			cout<<"    --help                     Display this help and exit"<<endl;
			cout<<"    --config <file.properties> Use the specified config file"<<endl;
			cout<<"    --dump                     Dump available tasks and their configuration properties and exit"<<endl;
			cout<<"    --replay <update_op.properties> Replay the failed update operation"<<endl;
		}
		else if (cmd_line.IsDump())
		{
			runner.DumpProperties(cout);
		}
		else if (cmd_line.IsReplay())
		{
			runner.Replay(cout, &cmd_line.GetReplayOperationConfigurator());
		}
		else {
			runner.Run(cout);
			cout<<"Done..."<<endl;
		}
	}
	catch (MemoriaException e)
	{
		cerr<<e.source()<<" ERROR: "<<e.message()<<endl;
	}
}
