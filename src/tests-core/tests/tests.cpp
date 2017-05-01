
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


//#include "packed/tree/packed_tree_test_suite.hpp"
//#include "packed/louds/packed_louds_suite.hpp"
//#include "packed/louds_cardinal/packed_lcardinal_suite.hpp"
//#include "packed/array/packed_array_test_suite.hpp"
//#include "packed/wavelet_tree/packed_wtree_suite.hpp"


// #include "prototype/bt/bt_test_suite.hpp"
// #include "prototype/btfl/btfl_test_suite.hpp"

// 
// 
// #include "multimap/multimap_test_suite.hpp"

//#include "sequence/sequence_test_suite.hpp"
//#include "labeled_tree/ltree_test_suite.hpp"
//#include "vector_tree/vtree_test_suite.hpp"
//#include "wt/wt_test_suite.hpp"



#include <memoria/v1/tools/cmdline.hpp>
#include <memoria/v1/tools/tools.hpp>
#include <memoria/v1/tools/tests.hpp>
#include <memoria/v1/tools/dump.hpp>
#include <memoria/v1/core/tools/terminal.hpp>







#include <iostream>


using namespace std;
using namespace memoria::v1;
using namespace memoria::v1::tools;



const char* DESCRIPTION = "Run Memoria regression tests with specified configuration";
const char* CFG_FILE    = "tests.properties";

int main(int argc, const char** argv, const char** envp)
{
    Term::init(argc, argv, envp);

    try {
        CmdLine cmd_line(argc, argv, envp, CFG_FILE, CmdLine::REPLAY);

        Int seed = cmd_line.getConfigurator().getValue<Int>("seed", -1);

        MemoriaTestRunner& runner = tests_runner();

        runner.setSeed(seed);

        runner.setRunCount(cmd_line.getCount());


//        runner.registerTask(new PackedTreeTestSuite());
//        runner.registerTask(new PackedArrayTestSuite());
//        runner.registerTask(new PackedLoudsTestSuite());
//        runner.registerTask(new PackedLoudsCardinalTestSuite());
//        runner.registerTask(new PackedWaveletTreeTestSuite());

//        runner.registerTask(new BTTestSuite());
//        runner.registerTask(new BTFLTestSuite());
        
//        runner.registerTask(new SequenceTestSuite());
//        runner.registerTask(new LabeledTreeTestSuite());
//        runner.registerTask(new WTTestSuite());

//        runner.registerTask(new VTreeTestSuite());
//        runner.registerTask(new MultiMapTestSuite());

        runner.Configure(&cmd_line.getConfigurator());

        if (cmd_line.IsHelp())
        {
            cout << endl;
            cout << "Description: "<<DESCRIPTION << endl;
            cout << "Usage: " << cmd_line.getImageName() << " [options]" << endl;
            cout << "    --help                           Display this help and exit" << endl;
            cout << "    --count N                        Run all tests N times" << endl;
            cout << "    --config <file.properties>       Use the specified config file" << endl;
            cout << "    --list                           "
                 << "List available tasks and their configuration properties and exit" << endl;
            cout << "    --dump <allocator.dump>          Dump allocator's content to disk" << endl;
            cout << "    --replay <update_op.properties>  Replay the failed update operation" << endl;
            cout << "    --out <output folder>            Path where tests output will be put. "
                 << "(It will be recreated if already exists)" << endl;
            cout << "    --coverage <small|normal|large>  Test coverage. Default is normal." << endl;
            cout << "    --coverage-size N                Test coverage size, N >= 1. Default is 1" << endl;
        }
        else if (cmd_line.IsList())
        {
            runner.dumpProperties(cout);
        }
        else if (cmd_line.IsReplay())
        {
            runner.Replay(cout, cmd_line.getReplayFile());
            return 0;
        }
        else if (cmd_line.IsDump())
        {
            DumpAllocator(cmd_line.getDumpFileName());
        }
        else {
            if (seed >= 0) {
                cout<<"Seed: "<<seed<<endl;
            }

            cout<<"Coverage: "<<cmd_line.getConfigurator().getValue<String>("coverage", "small")<<", Size: "<<cmd_line.getConfigurator().getValue<String>("coverage_size", "1")<<endl;

            String default_output_folder = cmd_line.getImageName()+".out";

            String output_folder = (cmd_line.getOutFolder() != NULL) ? cmd_line.getOutFolder() : default_output_folder;

            runner.setOutput(output_folder);

            Int failed = runner.Run();
            cout<<"Done..."<<endl;

            return failed;
        }
    }
    catch (MemoriaThrowable& e)
    {
        cerr<<e.source()<<" ERROR: "<<e<<endl;
    }

    return 1;
}
