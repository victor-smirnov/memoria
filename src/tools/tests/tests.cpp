
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "ctr/ctr_test_suite.hpp"

#include "map/map_test_suite.hpp"

#include "packed_map/pmap_test_suite.hpp"

#include "sum_set_batch/sum_tree_test_suite.hpp"

#include "vector/vector_test_suite.hpp"

#include "vector_map/vector_map_test_suite.hpp"

#include "template/task.hpp"

#include <memoria/tools/cmdline.hpp>

#include <iostream>

#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

using namespace std;
using namespace memoria;

MEMORIA_INIT();

const char* DESCRIPTION = "Run Memoria regression tests with specified configuration";
const char* CFG_FILE	= "tests.properties";

void sighandler(int signum)
{
	cout<<"SigSegv!"<<endl;
	throw MemoriaSigSegv(MEMORIA_SOURCE, "Segment violation");
}

int main(int argc, const char** argv, const char** envp)
{
	signal(SIGSEGV, sighandler);

	try {
		CmdLine cmd_line(argc, argv, envp, CFG_FILE, CmdLine::REPLAY);

		//FIXME: C++11 RNG seed doesn't work
		//Seed(GetTimeInMillis());
		//SeedBI(GetTimeInMillis());

		Int default_seed = GetTimeInMillis() % 10000;

		Int seed = cmd_line.GetConfigurator().GetValue<Int>("seed", default_seed);

		// Emulate seed;
		for (Int c = 0; c < seed; c++)
		{
			GetRandom();
			GetBIRandom();
		}

		MemoriaTestRunner runner;

		runner.SetRunCount(cmd_line.GetCount());

		// add test suits to the runner;

		runner.RegisterTask(new CtrTestSuite());
		runner.RegisterTask(new MapTestSuite());
		runner.RegisterTask(new PackedMapTestSuite());
		runner.RegisterTask(new SumTreeTestSuite());
		runner.RegisterTask(new VectorTestSuite());
		runner.RegisterTask(new VectorMapTestSuite());

		runner.Configure(&cmd_line.GetConfigurator());

		if (cmd_line.IsHelp())
		{
			cout<<endl;
			cout<<"Description: "<<DESCRIPTION<<endl;
			cout<<"Usage: "<<cmd_line.GetImageName()<<" [options]"<<endl;
			cout<<"    --help                     Display this help and exit"<<endl;
			cout<<"    --count N 		   		  Run all tests N times"<<endl;
			cout<<"    --config <file.properties> Use the specified config file"<<endl;
			cout<<"    --list                     List available tasks and their configuration properties and exit"<<endl;
			cout<<"    --replay <update_op.properties> Replay the failed update operation"<<endl;
			cout<<"    --out <output folder> 		   Path where tests output will be put. (It will be recreated if already exists)"<<endl;
		}
		else if (cmd_line.IsList())
		{
			runner.DumpProperties(cout);
		}
		else if (cmd_line.IsReplay())
		{
			runner.Replay(cout, cmd_line.GetReplayFile());
			return 0;
		}
		else {
			cout<<"Seed: "<<seed<<endl;

			String default_output_folder = cmd_line.GetImageName()+".out";

			String output_folder = (cmd_line.GetOutFolder() != NULL) ? cmd_line.GetOutFolder() : default_output_folder;

			runner.SetOutput(output_folder);

			Int failed = runner.Run();
			cout<<"Done..."<<endl;
			return failed;
		}
	}
	catch (MemoriaThrowable e)
	{
		cerr<<e.source()<<" ERROR: "<<e<<endl;
	}

	return 1;
}
