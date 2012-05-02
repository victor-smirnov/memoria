
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef TOOLS_BENCHMARKS_BENCHMARK_GROUPS_HPP_
#define TOOLS_BENCHMARKS_BENCHMARK_GROUPS_HPP_

#include <memoria/tools/benchmarks.hpp>

#include "packed_tree/pset_find_mem.hpp"
#include "packed_tree/pset_find_size.hpp"

#include "stl/stlset_find_mem.hpp"
#include "stl/stlset_find_size.hpp"
#include "stl/stlset_scan.hpp"
#include "stl/stluset_find_mem.hpp"
#include "stl/stluset_find_size.hpp"
#include "stl/stl_vector_read.hpp"
#include "stl/stl_vector_insert.hpp"


#include "set/set_scan.hpp"
#include "set/set_append.hpp"
#include "set/set_commit_append.hpp"
#include "set/set_commit_random.hpp"
#include "set/set_create_batch.hpp"
#include "set/set_find.hpp"
#include "set/set_insert.hpp"
#include "set/set_insert_batch.hpp"

#include "vector/vector_append.hpp"
#include "vector/vector_insert_batch.hpp"
#include "vector/vector_read.hpp"
#include "vector/vector_sequential_read.hpp"
#include "vector/vector_random_insert.hpp"

#include "vector_map/vector_map_append.hpp"
#include "vector_map/vector_map_random_insert.hpp"
#include "vector_map/vector_map_random_read.hpp"
#include "vector_map/vector_map_sequential_read.hpp"

#include "misc/memmove.hpp"

namespace memoria {

class LogXScaleGnuplotGraph: public GnuplotGraph {
	BigInt value_;
public:

	LogXScaleGnuplotGraph(StringRef name, BigInt value = 2): GnuplotGraph(name), value_(value) {}

	virtual ~LogXScaleGnuplotGraph() throw () {}

	virtual BigInt NextTime()
	{
		BigInt tmp = current_time;
		current_time *= value_;
		return tmp;
	}
};


class PackedSetMemGraph: public LogXScaleGnuplotGraph {
public:
	PackedSetMemGraph(): LogXScaleGnuplotGraph("PSetMem")
	{
		title 	= "PackedTree random read performance,\\n1 million random reads";
		xtitle 	= "Memory Block Size, Kb";
		ytitle	= "Execution Time, ms";

		time_start 	= 1024;
		time_stop	= 256*1024*1024;

		xunit 		= 1024;

		RegisterTask(new PSetMemBenchmark<2>());
		RegisterTask(new PSetMemBenchmark<4>());
		RegisterTask(new PSetMemBenchmark<8>());
		RegisterTask(new PSetMemBenchmark<16>());
		RegisterTask(new PSetMemBenchmark<32>());
		RegisterTask(new PSetMemBenchmark<64>());
	}
};


class PackedSetSizeGraph: public LogXScaleGnuplotGraph {
public:
	PackedSetSizeGraph(): LogXScaleGnuplotGraph("PSetSize")
	{
		title 	= "PackedTree random read performance,\\n1 million random reads";
		xtitle 	= "PackedTree Size, Elements";
		ytitle	= "Execution Time, ms";

		time_start 	= 128;
		time_stop	= 16*1024*1024;

		xunit 		= 1;

		logscale	= 10;

		RegisterTask(new PSetSizeBenchmark<2>());
		RegisterTask(new PSetSizeBenchmark<4>());
		RegisterTask(new PSetSizeBenchmark<8>());
		RegisterTask(new PSetSizeBenchmark<16>());
		RegisterTask(new PSetSizeBenchmark<32>());
		RegisterTask(new PSetSizeBenchmark<64>());
	}
};


class PackedSetStlSetMemGraph: public LogXScaleGnuplotGraph {
public:
	PackedSetStlSetMemGraph(): LogXScaleGnuplotGraph("PSetStlSetMem")
	{
		title 	= "PackedSet vs STL Set random read performance,\\n1 million random reads";
		xtitle 	= "Memory Block Size, Kb";
		ytitle	= "Execution Time, ms";

		time_start 	= 1024;
		time_stop	= 256*1024*1024;

		xunit 		= 1024;

		logscale	= 10;

		RegisterTask(new PSetMemBenchmark<16>());
		RegisterTask(new StlSetMemBenchmark());
	}
};


class PackedSetStlSetSizeGraph: public LogXScaleGnuplotGraph {
public:
	PackedSetStlSetSizeGraph(): LogXScaleGnuplotGraph("PSetStlSetSize")
	{
		title 	= "PackedSet vs STL Set random read performance,\\n1 million random reads";
		xtitle 	= "Number of Elements";
		ytitle	= "Execution Time, ms";

		time_start 	= 128;
		time_stop	= 16*1024*1024;

		xunit 		= 1;

		logscale	= 10;

		RegisterTask(new PSetSizeBenchmark<16>());
		RegisterTask(new StlSetSizeBenchmark());
	}
};


class PackedSetStlUSetSizeGraph: public LogXScaleGnuplotGraph {
public:
	PackedSetStlUSetSizeGraph(): LogXScaleGnuplotGraph("PSetStlUSetSize")
	{
		title 	= "PackedSet vs STL unordered_set random read performance,\\n1 million random reads";
		xtitle 	= "Number of Elements";
		ytitle	= "Execution Time, ms";

		time_start 	= 128;
		time_stop	= 16*1024*1024;

		xunit 		= 1;

		logscale	= 10;

		RegisterTask(new PSetSizeBenchmark<16>());
		RegisterTask(new StlUSetSizeBenchmark());
	}
};


class ScanSpeedGraph: public LogXScaleGnuplotGraph {
public:
	ScanSpeedGraph(): LogXScaleGnuplotGraph("ScanSpeed")
	{
		title 	= "Memoria Set vs STL set linear read performance,\\n1 million reads";
		xtitle 	= "Number of Elements";
		ytitle	= "Execution Time, ms";

		time_start 	= 128;
		time_stop	= 16*1024*1024;

		xunit 		= 1;

		logscale	= 10;

		RegisterTask(new SetScanBenchmark());
		RegisterTask(new StlSetScanBenchmark());
	}
};

class MemmoveGraph: public LogXScaleGnuplotGraph {
public:
	MemmoveGraph(): LogXScaleGnuplotGraph("MemMove")
	{
		title 	= "Memmove performance,\\n1 million moves of size up to 4K bytes";
		xtitle 	= "Memory Block Size, Kb";
		ytitle	= "Execution Time, ms";

		time_start 	= 4096;
		time_stop	= 256*1024*1024;

		xunit 		= 1024;

		logscale	= 10;

		RegisterTask(new MemmoveBenchmark());
	}
};


class TestGraph: public LogXScaleGnuplotGraph {
public:
	TestGraph(): LogXScaleGnuplotGraph("Test")
	{
		title 	= "Something vs Something,\\n1 million operations";
		xtitle 	= "Something";
		ytitle	= "Execution Time, ms";

		time_start 	= 8;
		time_stop	= 1024*1024;

		xunit 		= 1;
		yunit		= 1;

		logscale	= 0;

		RegisterTask(new VectorMapAppendBenchmark());
		RegisterTask(new VectorMapRandomInsertBenchmark());
		RegisterTask(new VectorAppendBenchmark());
		RegisterTask(new VectorRandomInsertBenchmark());
	}
};


}


#endif /* BENCHMARK_GROUPS_HPP_ */
