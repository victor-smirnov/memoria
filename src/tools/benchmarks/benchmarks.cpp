
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

const char* DESCRIPTION = "Run Memoria benchmarks with specified configuration";
const char* CFG_FILE    = "benchmarks.properties";

int main(int argc, const char** argv, const char** envp)
{
    MEMORIA_INIT(SmallProfile<>);

    try {
        CmdLine cmd_line(argc, argv, envp, CFG_FILE, CmdLine::NONE);

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

        runner.setRunCount(cmd_line.getCount());

        // add tasks to the runner;


        runner.registerTask(new MemoryThroughputGraph());

        runner.registerTask(new PackedSetMemGraph());

        runner.registerTask(new SetRandomReadGraph());
        runner.registerTask(new SetLinearReadGraph());
        runner.registerTask(new SetRandominsertGraph());
        runner.registerTask(new SetCommitRateGraph());


        runner.registerTask(new MemmoveGraph());

        runner.registerTask(new VectorRandomSmallReadGraph());
        runner.registerTask(new VectorReadGraph());
        runner.registerTask(new VectorInsertGraph());


        runner.registerTask(new VectorMapRandomGraph());
        runner.registerTask(new VectorMapLinearGraph());
        runner.registerTask(new VectorMapReadOverheadGraph());
        runner.registerTask(new VectorMapBatchinsertGraph());

        runner.registerTask(new TestGraph());

        runner.Configure(&cmd_line.getConfigurator());

        if (cmd_line.IsHelp())
        {
            cout<<endl;
            cout<<"Description: "<<DESCRIPTION<<endl;
            cout<<"Usage: "<<cmd_line.getImageName()<<" [options]"<<endl;
            cout<<"    --help                     Display this help and exit"<<endl;
            cout<<"    --count N                  Run all tests N times"<<endl;
            cout<<"    --config <file.properties> Use the specified config file"<<endl;
            cout<<"    --list                     List available tasks and their configuration properties and exit"<<endl;
            cout<<"    --out <output folder>      Path where tests output will be put. "
                <<"(It will be recreated if already exists)"<<endl;
        }
        else if (cmd_line.IsList())
        {
            runner.dumpProperties(cout);
        }
        else {
            cout<<"Seed: "<<seed<<endl;

            String default_output_folder = cmd_line.getImageName()+".out";

            String output_folder = (cmd_line.getOutFolder() != NULL) ? cmd_line.getOutFolder() : default_output_folder;

            runner.setOutput(output_folder);

            runner.Run();
            cout<<"Done..."<<endl;

            return 0;
        }
    }
    catch (MemoriaThrowable e)
    {
        cerr<<e.source()<<" ERROR: "<<e<<endl;
    }

    return 1;
}
