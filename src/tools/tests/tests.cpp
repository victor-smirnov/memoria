
// Copyright Victor Smirnov 2012-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//#include "symbol_seq/symseq_test_suite.hpp"
//#include "bitmap/bitmap_test_suite.hpp"
//
//#include "packed/allocator/palloc_test_suite.hpp"
//#include "packed/tree/packed_tree_test_suite.hpp"
//#include "packed/array/packed_array_test_suite.hpp"
//#include "packed/sequence/packed_seq_suite.hpp"
//#include "packed/louds/packed_louds_suite.hpp"
//#include "packed/louds_cardinal/packed_lcardinal_suite.hpp"
//#include "packed/wavelet_tree/packed_wtree_suite.hpp"
//
//#include "ctr/ctr_test_suite.hpp"
#include "map/map_test_suite.hpp"
#include "vector/vector_test_suite.hpp"
//#include "vector_map/vectormap_test_suite.hpp"
//#include "dbl_map/dblmap_test_suite.hpp"
#include "sequence/sequence_test_suite.hpp"
//#include "labeled_tree/ltree_test_suite.hpp"
//#include "vector_tree/vtree_test_suite.hpp"
//#include "wt/wt_test_suite.hpp"

//#include "file_allocator/file_alloc_test_suite.hpp"
//#include "mvcc/mvcc_test_suite.hpp"

#include <memoria/tools/cmdline.hpp>
#include <memoria/tools/tools.hpp>
#include <memoria/tools/tests.hpp>
#include <memoria/core/tools/terminal.hpp>
#include "dump.hpp"

#include <iostream>

using namespace std;
using namespace memoria;
using namespace memoria::tools;



const char* DESCRIPTION = "Run Memoria regression tests with specified configuration";
const char* CFG_FILE    = "tests.properties";

int main(int argc, const char** argv, const char** envp)
{
    MEMORIA_INIT(SmallProfile<>);

    Term::init(argc, argv, envp);

    try {
    	SCtrTF<MapX<BigInt, BigInt>>::Type::initMetadata();

    	CmdLine cmd_line(argc, argv, envp, CFG_FILE, CmdLine::REPLAY);

        Int default_seed = getTimeInMillis() % 100000;

        Int seed = cmd_line.getConfigurator().getValue<Int>("seed", default_seed);

        // Emulate seed;
        for (Int c = 0; c < seed; c++)
        {
            getRandom();
            getBIRandom();
        }

        MemoriaTestRunner runner;

        runner.setRunCount(cmd_line.getCount());
//
//        runner.registerTask(new BitmapTestSuite());
////
//        runner.registerTask(new PackedAllocatorTestSuite());
//        runner.registerTask(new PackedTreeTestSuite());
//        runner.registerTask(new PackedArrayTestSuite());
//        runner.registerTask(new PackedSequenceTestSuite());
//        runner.registerTask(new PackedLoudsTestSuite());
//        runner.registerTask(new PackedLoudsCardinalTestSuite());
//        runner.registerTask(new PackedWaveletTreeTestSuite());
////
//        runner.registerTask(new SymbolSeqTestSuite());
////
//        runner.registerTask(new CtrTestSuite());
        runner.registerTask(new MapTestSuite());
        runner.registerTask(new VectorTestSuite());
//        runner.registerTask(new VectorMapTestSuite());
//        runner.registerTask(new DblMapTestSuite());
        runner.registerTask(new SequenceTestSuite());
//        runner.registerTask(new LabeledTreeTestSuite());
//        runner.registerTask(new FileAllocatorTestSuite());
//        runner.registerTask(new VTreeTestSuite());
//        runner.registerTask(new WTTestSuite());
//
//
//        runner.registerTask(new MVCCTestSuite());


        runner.Configure(&cmd_line.getConfigurator());

        if (cmd_line.IsHelp())
        {
            cout<<endl;
            cout<<"Description: "<<DESCRIPTION<<endl;
            cout<<"Usage: "<<cmd_line.getImageName()<<" [options]"<<endl;
            cout<<"    --help                           Display this help and exit"<<endl;
            cout<<"    --count N                        Run all tests N times"<<endl;
            cout<<"    --config <file.properties>       Use the specified config file"<<endl;
            cout<<"    --list                           "
                <<"List available tasks and their configuration properties and exit"<<endl;
            cout<<"    --dump <allocator.dump>          Dump allocator's content to disk"<<endl;
            cout<<"    --replay <update_op.properties>  Replay the failed update operation"<<endl;
            cout<<"    --out <output folder>            Path where tests output will be put. "
                <<"(It will be recreated if already exists)"<<endl;
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
            cout<<"Seed: "<<seed<<endl;

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
