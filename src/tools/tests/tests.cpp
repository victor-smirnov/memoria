
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include "tree_map.hpp"

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

		TestRunner runner;

		// add tasks to the runner;

		runner.RegisterTask(new TreeMapTestTask());

		runner.Configure(&cmd_line.GetConfigurator());

		if (cmd_line.IsHelp())
		{
			cout<<endl;
			cout<<"Description: "<<DESCRIPTION<<endl;
			cout<<"Usage: "<<cmd_line.GetImageName()<<" [--help] [--dump] [--config <file.properties>]"<<endl;
			cout<<"    --help                     Display this help and exit"<<endl;
			cout<<"    --config <file.properties> Use the specified config file"<<endl;
			cout<<"    --dump                     Dump available tasks and their configuration properties and exit"<<endl;
			cout<<"    --replay <update_op.properties> <dump-file> Replay the failed update operation on the allocator dump"<<endl;
		}
		else if (cmd_line.IsDump())
		{
			runner.DumpProperties(cout);
		}
		else if (cmd_line.IsReplay())
		{
			runner.Replay(&cmd_line.GetReplayOperationConfigurator(), cmd_line.GetDumpFileName());
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
