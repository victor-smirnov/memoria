
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/tools/benchmarks.hpp>
#include <memoria/tools/cmdline.hpp>
#include <memoria/memoria.hpp>

#include "benchmark_groups.hpp"

#include <iostream>

#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#include <stdio.h>
#include <stdlib.h>

using namespace std;
using namespace memoria;

MEMORIA_INIT();

const char* DESCRIPTION = "Run Memoria benchmarks with specified configuration";
const char* CFG_FILE	= "benchmarks.properties";

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
		CmdLine cmd_line(argc, argv, envp, CFG_FILE, CmdLine::NONE);

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

		MemoriaTaskRunner runner;

		runner.SetRunCount(cmd_line.GetCount());

		// add tasks to the runner;


		runner.RegisterTask(new PackedSetMemGraph());
		runner.RegisterTask(new PackedSetSizeGraph());
		runner.RegisterTask(new PackedSetStlSetMemGraph());
		runner.RegisterTask(new PackedSetStlSetSizeGraph());
		runner.RegisterTask(new PackedSetStlUSetSizeGraph());
		runner.RegisterTask(new ScanSpeedGraph());
		runner.RegisterTask(new MemmoveGraph());
		runner.RegisterTask(new SetBatchUpdateGraph());
		runner.RegisterTask(new VectorMapGraph());
		runner.RegisterTask(new TestGraph());

		runner.Configure(&cmd_line.GetConfigurator());

		if (cmd_line.IsHelp())
		{
			cout<<endl;
			cout<<"Description: "<<DESCRIPTION<<endl;
			cout<<"Usage: "<<cmd_line.GetImageName()<<" [options]"<<endl;
			cout<<"    --help                     Display this help and exit"<<endl;
			cout<<"    --count N                  Run all tests N times"<<endl;
			cout<<"    --config <file.properties> Use the specified config file"<<endl;
			cout<<"    --list                     List available tasks and their configuration properties and exit"<<endl;
			cout<<"    --out <output folder>      Path where tests output will be put. (It will be recreated if already exists)"<<endl;
		}
		else if (cmd_line.IsList())
		{
			runner.DumpProperties(cout);
		}
		else {
			cout<<"Seed: "<<seed<<endl;

			String default_output_folder = cmd_line.GetImageName()+".out";

			String output_folder = (cmd_line.GetOutFolder() != NULL) ? cmd_line.GetOutFolder() : default_output_folder;

//			bool clear = cmd_line.GetConfigurator().GetValue<bool>("clear", true);

//			File outf(output_folder);
//			if (outf.IsExists() && clear)
//			{
//				if (outf.IsDirectory())
//				{
//					if (!outf.DelTree())
//					{
//						throw MemoriaException(MEMORIA_SOURCE, "Can't remove folder: " + String(cmd_line.GetOutFolder()));
//					}
//				}
//				else if (!outf.Delete())
//				{
//					throw MemoriaException(MEMORIA_SOURCE, "Can't remove file: " + String(cmd_line.GetOutFolder()));
//				}
//			}
//
//			outf.MkDirs();

			runner.SetOutput(output_folder);

			runner.Run();
			cout<<"Done..."<<endl;

			return 0;
		}
	}
	catch (MemoriaException e)
	{
		cerr<<e.source()<<" ERROR: "<<e.message()<<endl;
	}

	return 1;
}
