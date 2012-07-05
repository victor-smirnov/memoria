
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include "create_ctr.hpp"
#include "copy_ctr.hpp"
#include "map.hpp"
#include "vector_map.hpp"

#include "empty_example.hpp"

#include <memoria/tools/examples.hpp>

#include <memoria/tools/cmdline.hpp>

#include <memoria/memoria.hpp>

#include <iostream>

#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

using namespace std;
using namespace memoria;

MEMORIA_INIT();

const char* DESCRIPTION = "Run Memoria examples with specified configuration";
const char* CFG_FILE	= "examples.properties";

void sighandler(int signum)
{
	cout<<"SigSegv!"<<endl;
	throw MemoriaSigSegv(MEMORIA_SOURCE, "Segment violation");
}

int main(int argc, const char** argv, const char** envp)
{
	signal(SIGSEGV, sighandler);

	SmallCtrTypeFactory::Factory<Root>::Type::Init();

	try {
		CmdLine cmd_line(argc, argv, envp, CFG_FILE, CmdLine::REPLAY);

		//FIXME: C++11 RNG seed doesn't work
		//Seed(getTimeInMillis());
		//SeedBI(getTimeInMillis());

		Int default_seed = getTimeInMillis() % 10000;

		Int seed = cmd_line.getConfigurator().getValue<Int>("seed", default_seed);

		// Emulate seed;
		for (Int c = 0; c < seed; c++)
		{
			getRandom();
			getBIRandom();
		}

		MemoriaTaskRunner runner;

		// add tasks to the runner;

		TaskGroup* examples = new TaskGroup("Examples");

		examples->RegisterTask(new CreateCtrExample());
		examples->RegisterTask(new CopyCtrExample());
		examples->RegisterTask(new MapExample());
		examples->RegisterTask(new VectorMapExample());
		examples->RegisterTask(new EmptyExample());

		runner.RegisterTask(examples);

		runner.Configure(&cmd_line.getConfigurator());

		runner.setRunCount(cmd_line.getCount());

		if (cmd_line.IsHelp())
		{
			cout<<endl;
			cout<<"Description: "<<DESCRIPTION<<endl;
			cout<<"Usage: "<<cmd_line.getImageName()<<" [options]"<<endl;
			cout<<"    --help                     Display this help and exit"<<endl;
			cout<<"    --count N 		   		  Run all tests N times"<<endl;
			cout<<"    --config <file.properties> Use the specified config file"<<endl;
			cout<<"    --list                     List available tasks and their configuration properties and exit"<<endl;
			cout<<"    --out <output folder> 		   Path where tests output will be put. (It will be recreated if already exists)"<<endl;
		}
		else if (cmd_line.IsList())
		{
			runner.DumpProperties(cout);
		}
		else {
			cout<<"Seed: "<<seed<<endl;

			String default_output_folder = cmd_line.getImageName()+".out";

			String output_folder = (cmd_line.getOutFolder() != NULL) ? cmd_line.getOutFolder() : default_output_folder;

			runner.setOutput(output_folder);

			Int failed = 0;
			runner.Run();
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
