
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
#include "stl/stl_vector_random_read.hpp"
#include "stl/stl_vector_linear_read.hpp"
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
#include "vector/vector_random_read.hpp"
#include "vector/vector_sequential_read.hpp"
#include "vector/vector_random_insert.hpp"

#include "vector_map/vector_map_append.hpp"
#include "vector_map/vector_map_batch_insert.hpp"
#include "vector_map/vector_map_random_insert.hpp"
#include "vector_map/vector_map_random_read.hpp"
#include "vector_map/vector_map_sequential_read.hpp"

#include "misc/memmove.hpp"


/**
 * Memoria benchmark suite chart list:
 *
 * 1. Packedset performance (branching factor, tree size)
 * 		- random read + stl		 	(1.1)
 *
 * 2. Memoria set
 * 		- random read (tree size)	(2.1)
 * 		- linear read 				(2.1)
 * 		- random insert				(2.2)
 * 		- linear insert				(2.2)
 * 		- batch create (batch size)	(2.3)
 * 		- batch random insert		(2.4)
 * 		- batch linear insert		(2.4)
 * 		- commit overhead random	(2.5)
 * 		- commit overhead linear	(2.5)
 * 		+ multicore ?
 *
 * 3. Memmove performance (multicore)							(3.1)
 *
 * 4. Vector
 *		- random read (small block, vector size)				(4.1)
 *		- random read (fixed vector size, by block size)		(4.2)
 *		- linear read (fixed vector size, by block size)		(4.2)
 *		- random insert (fixed vector size, by block size)		(4.3)
 *		- linear insert (fixed vector size, by block size)		(4.3)
 *		- random remove (fixed vector size, by block size)		(4.3)
 *		+ multicore ?
 *
 * 5. VectorMap
 * 		- random read (value size)								(5.1)
 * 		- linear read (value size)								(5.1)
 * 		- random insert (value size)							(5.2)
 * 		- linear insert											(5.2)
 * 		- random batch insert									(5.3)
 * 		- linear batch insert									(5.4)
 * 		+ multicore ?
 *
 * 6.	Serialization test
 * 		- set 													(6.1)
 * 		- Vector												(6.2)
 * 		- VectorMap												(6.3)
 */


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

class MemoryThroughputGraph: public LogXScaleGnuplotGraph {
public:
	MemoryThroughputGraph(): LogXScaleGnuplotGraph("MemThroughput")
	{
		title 	= "Random/Ordered DRAM Memory Performance and Throughput,\n8 byte data";
		xtitle 	= "Memory Block Size, Kb";

		agenda_location = "top right";

		time_start 	= 1024;
		time_stop	= 128*1024*1024;

		operations	= time_stop/8;

		xunit 		= 1024;

		y2 			= true;

		AddGraph(new StlVectorRandomReadBenchmark("Random"), GraphData("Random access performance", "Random access throughput"));
		AddGraph(new StlVectorLinearReadBenchmark("Sequential"), GraphData("Sequential access performance", "Sequential access throughput"));
	}
};


class PackedsetMemGraph: public LogXScaleGnuplotGraph {
public:
	PackedsetMemGraph(): LogXScaleGnuplotGraph("PsetMem")
	{
		title 	= "PackedTree random read performance,\\n1 million random reads";
		xtitle 	= "Memory Block Size, Kb";

		agenda_location = "top right";

		time_start 	= 1024;
		time_stop	= 256*1024*1024;

		xunit 		= 1024;

		AddGraph(new PsetMemBenchmark<2>(), GraphData("PackedTree<BigInt>, 2 children"));
		AddGraph(new PsetMemBenchmark<4>(), GraphData("PackedTree<BigInt>, 4 children"));
		AddGraph(new PsetMemBenchmark<8>(), GraphData("PackedTree<BigInt>, 8 children"));
		AddGraph(new PsetMemBenchmark<16>(), GraphData("PackedTree<BigInt>, 16 children"));
		AddGraph(new PsetMemBenchmark<32>(), GraphData("PackedTree<BigInt>, 32 children"));
		AddGraph(new PsetMemBenchmark<64>(), GraphData("PackedTree<BigInt>, 64 children"));

		AddGraph(new StlsetMemBenchmark("StlsetMem"), GraphData("std::set, 2 children"));
	}
};


class SetRandomReadGraph: public LogXScaleGnuplotGraph {
public:
	SetRandomReadGraph(): LogXScaleGnuplotGraph("setRandomRead")
	{
		title 	= "Packedset<BigInt> vs set<BigInt> vs std::set<BigInt> Random Read Performance,\\n1 million reads";
		xtitle 	= "Number of Elements";

		time_start 	= 128;
		time_stop	= 16*1024*1024;

		xunit 		= 1;

		logscale	= 10;

		AddGraph(new PsetSizeBenchmark<16>(), GraphData("Packedset"));
		AddGraph(new setFindRandomBenchmark("findRandom"), GraphData("Memoria set"));
		AddGraph(new StlsetSizeBenchmark("StlfindRandom"), GraphData("std::set"));
	}
};



class SetLinearReadGraph: public LogXScaleGnuplotGraph {
public:
	SetLinearReadGraph(): LogXScaleGnuplotGraph("setLinearRead")
	{
		title 	= "Memoria set<BigInt> vs std::set<BigInt> Sequential Read Performance,\\n16 million reads";
		xtitle 	= "Number of Elements";

		time_start 	= 128;
		time_stop	= 16*1024*1024;

		operations	= 16*1024*1024;

		xunit 		= 1;

		logscale	= 10;

		AddGraph(new setScanBenchmark("setScan"), GraphData("Memoria set"));
		AddGraph(new StlsetScanBenchmark("StlsetScan"), GraphData("std::set"));
	}
};



class SetRandominsertGraph: public LogXScaleGnuplotGraph {
public:
	SetRandominsertGraph(): LogXScaleGnuplotGraph("setinsert")
	{
		title 	= "set<BigInt> Batch insert Performance";
		xtitle 	= "Batch size, Elements";

		time_start 	= 1;
		time_stop	= 100000;

		xunit 		= 1;

		logscale	= 10;

		AddGraph(new setinsertBatchBenchmark("insertBatch"), GraphData("Random insert"));
		AddGraph(new SetappendBatchBenchmark("appendBatch"), GraphData("Sequential append"));
	}
};


class SetCommitRateGraph: public LogXScaleGnuplotGraph {
public:
	SetCommitRateGraph(): LogXScaleGnuplotGraph("setCommitRate", 10)
	{
		title 	= "set<BigInt> Commit Performance,\\ninsert/append 1M keys";
		xtitle 	= "Commit Batch size, Elements";

		time_start 	= 1;
		time_stop	= 100000;

		xunit 		= 1;

		logscale	= 10;

		AddGraph(new setCommitRandomBenchmark("Random"), GraphData("Random insert"));
		AddGraph(new setCommitappendBenchmark("append"), GraphData("Sequential append"));
	}
};


class SetBatchUpdateGraph: public LogXScaleGnuplotGraph {
public:
	SetBatchUpdateGraph(): LogXScaleGnuplotGraph("setBatchUpdate", 10)
	{
		title 	= "insert 16M keys into Memoria set";
		xtitle 	= "Batch size";
		ytitle	= "Performance, insertions/sec";

		time_start 	= 1;
		time_stop	= 100000;

		xunit 		= 1;
		yunit		= 1000;

		logscale	= 10;

		RegisterTask(new setinsertBatchBenchmark("insert"));
		RegisterTask(new SetappendBatchBenchmark("append"));
	}
};


class MemmoveGraph: public LogXScaleGnuplotGraph {
public:
	MemmoveGraph(): LogXScaleGnuplotGraph("MemMove")
	{
		title 	= "Memmove Performance,\\n1 million moves of size up to 4K bytes";
		xtitle 	= "Memory Block Size, Kb";

		y2 		= true;

		time_start 	= 4096;
		time_stop	= 256*1024*1024;

		xunit 		= 1024;
		y2unit		= 1024*1024;

		logscale	= 2;

		AddGraph(new MemmoveBenchmark("MemMove"), GraphData("memmove() Performance", "Memory Throughput"));
	}
};

class VectorRandomSmallReadGraph: public LogXScaleGnuplotGraph {
public:
	VectorRandomSmallReadGraph(): LogXScaleGnuplotGraph("VectorRandomSmallRead")
	{
		title 	= "Memoria Vector<Byte> Read Performance, 8 Byte Blocks ,\\n1 million random reads";
		xtitle 	= "Vector Size, Kb";

		agenda_location = "top right";

		time_start 	= 1024;
		time_stop	= 256*1024*1024;

		operations = 32*1024*1024;

		y2			= true;

		xunit 		= 1024;
		y2unit		= 1024*1024;

		AddGraph(new VectorReadBenchmark("Read"), GraphData("Vector Performance", "Vector Throughput"));
	}
};


class VectorReadGraph: public LogXScaleGnuplotGraph {
public:
	VectorReadGraph(): LogXScaleGnuplotGraph("VectorRead")
	{
		title 	= "Vector Read Performance";
		xtitle 	= "Block Size, Bytes";

		time_start 	= 8;
		time_stop	= 256*1024;

		y2			= true;

		y2unit		= 1024*1024;

		agenda_location = "top left";

		AddGraph(new VectorRandomReadBenchmark("Random"), GraphData("Random Performance", "Random Throughput"));
		AddGraph(new VectorSequentialReadBenchmark("Sequential"), GraphData("Sequential Performance", "Sequential Throughput"));
	}
};


class VectorinsertGraph: public LogXScaleGnuplotGraph {
public:
	VectorinsertGraph(): LogXScaleGnuplotGraph("Vectorinsert")
	{
		title 	= "Vector insert Performance";
		xtitle 	= "Block Size, Bytes";

		agenda_location = "top left";

		y2tics_format	= "%g";

		time_start 	= 8;
		time_stop	= 256*1024;

		y2			= true;

		y2unit		= 1024*1024;

		AddGraph(new VectorRandominsertBenchmark("Random"), GraphData("Random insert Performance", "Random insert Throughput"));
		AddGraph(new VectorappendBenchmark("Sequential"), GraphData("Sequential append Performance", "Sequential append Throughput"));
	}
};



class VectorMapRandomGraph: public LogXScaleGnuplotGraph {
public:
	VectorMapRandomGraph(): LogXScaleGnuplotGraph("VectorMapRandom")
	{
		title 	= "VectorMap Random Performance";
		xtitle 	= "Value Size";

		time_start 	= 8;
		time_stop	= 1024*1024;

		logscale	= 2;

		y2			= true;

		AddGraph(new VectorMapRandominsertBenchmark("insert"), GraphData("insert Performance", "insert Throughput"));
		AddGraph(new VectorMapRandomReadBenchmark("Read"), GraphData("Read Performance", "Read Throughput"));
	}
};

class VectorMapLinearGraph: public LogXScaleGnuplotGraph {
public:
	VectorMapLinearGraph(): LogXScaleGnuplotGraph("VectorMapSequential")
	{
		title 	= "VectorMap Sequential Performance";
		xtitle 	= "Value Size";

		time_start 	= 8;
		time_stop	= 1024*1024;

		logscale	= 2;

		y2			= true;

		AddGraph(new VectorMapappendBenchmark("append"), GraphData("append Performance", "append Throughput"));
		AddGraph(new VectorMapSequentialReadBenchmark("Read"), GraphData("Read Performance", "Read Throughput"));
	}
};

class VectorMapReadOverheadGraph: public LogXScaleGnuplotGraph {
public:
	VectorMapReadOverheadGraph(): LogXScaleGnuplotGraph("VectorMapReadOverhead", 2)
	{
		title 	= "Memoria Vector/VectorMap Sequential Read Performance";
		xtitle 	= "Value/Block size, Bytes";
		ytitle	= "Performance, Reads/sec";

		y2 		= true;

		time_start 	= 8;
		time_stop	= 256*1024;

		xunit 		= 1;
		yunit		= 1;
		y2unit		= 1024*1024;

		logscale	= 2;

		AddGraph(new VectorSequentialReadBenchmark("Vector"), GraphData("Vector Performance", "Vector Throughput"));
		AddGraph(new VectorMapSequentialReadBenchmark("VectorMap"), GraphData("VectorMap Performance", "VectorMap Throughput"));
	}
};

class VectorMapBatchinsertGraph: public LogXScaleGnuplotGraph {
public:
	VectorMapBatchinsertGraph(): LogXScaleGnuplotGraph("VectorMapBatchinsert", 2)
	{
		title 	= "VectorMap Batch insert Performance";
		xtitle 	= "Batch Size, Elements";
		ytitle	= "Performance, insertions/sec";
		y2title	= "Throughput, MiB/sec";

		y2 		= true;

		agenda_location = "top left";

		time_start 	= 1;
		time_stop	= 128*1024;

		logscale	= 2;

		AddGraph(new VectorMapBatchinsertBenchmark("insert.128", 128), GraphData("VectorMap Performance, 128 bytes value", "VectorMap Throughput, 128 bytes value"));
	}
};

class TestGraph: public LogXScaleGnuplotGraph {
public:
	TestGraph(): LogXScaleGnuplotGraph("Test", 2)
	{
		title 	= "Memoria Vector/VectorMap Sequential Read Performance";
		xtitle 	= "Value/Block size, Bytes";
		ytitle	= "Performace, reads/sec";
		y2title	= "Throughput, MiB/sec";

		y2 		= true;

		time_start 	= 8;
		time_stop	= 256*1024;

		xunit 		= 1;
		yunit		= 1;
		y2unit		= 1024*1024;

		logscale	= 2;

//		AddGraph(new VectorSequentialReadBenchmark("Vector"), GraphData("Vector Performance", "Vector Throughput"));
//		AddGraph(new VectorMapSequentialReadBenchmark("VectorMap"), GraphData("VectorMap Performance", "VectorMap Throughput"));

		AddGraph(new StlsetSizeBenchmark("StlfindRandom"), GraphData("std::set"));
	}
};


}


#endif /* BENCHMARK_GROUPS_HPP_ */
