
// Copyright 2012 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.



#include <memoria/v1/tools/benchmarks.hpp>
#include <memoria/v1/tools/cmdline.hpp>
#include <memoria/v1/memoria.hpp>

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
    Term::init(argc, argv, envp);

    MEMORIA_INIT(DefaultProfile<>);

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

        MemoriaTaskRunner runner("Benchmarks");

        runner.setRunCount(cmd_line.getCount());

        // add tasks to the runner;


        runner.registerTask(new MemoryThroughputGraph());

        runner.registerTask(new PackedSetMemGraph());

//        runner.registerTask(new SetRandomReadGraph());
//        runner.registerTask(new SetLinearReadGraph());
//        runner.registerTask(new SetRandomBatchInsertGraph());
//        runner.registerTask(new SetCommitRateGraph());


        runner.registerTask(new MemmoveGraph());

//        runner.registerTask(new VectorRandomSmallReadGraph());
//        runner.registerTask(new VectorReadGraph());
//        runner.registerTask(new VectorInsertGraph());

//
//        runner.registerTask(new VectorMapRandomGraph());
//        runner.registerTask(new VectorMapLinearGraph());
//        runner.registerTask(new VectorMapReadOverheadGraph());
//        runner.registerTask(new VectorMapBatchinsertGraph());

//        runner.registerTask(new TestGraph());

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
