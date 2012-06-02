
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/core/pmap/packed_sum_tree.hpp>
#include <memoria/prototypes/bstree/tools.hpp>

#include <memoria/tools/tools.hpp>

#include <iostream>

#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

using namespace std;
using namespace memoria;
using namespace memoria::btree;

template <typename Key_, typename Value_ = EmptyValue>
struct PMapFindTypes {
	typedef Key_ 						Key;
	typedef Key_ 						IndexKey;
	typedef Value_						Value;

	static const Int Blocks 			= 1;
	static const Int BranchingFactor	= 32;

	typedef Accumulators<Key, Blocks> 	Accumulator;
};

typedef PMapFindTypes<BigInt> 			Types;

typedef Types::Accumulator				Accumulator;
typedef Types::Key						Key;
typedef Types::Value					Value;

typedef PackedSumTree<Types> 			PMap;

void FillPMap(PMap* map, Int size)
{
	map->key(0, 0) = 0;

	for (Int c = 1; c < size; c++)
	{
		map->key(0, c) = 1;
	}

	map->size() = size;

	map->Reindex(0);
}


void Benchmark(Int size, Int average = 10)
{
	Int buffer_size 	= size*1024;

	Byte* buffer 		= T2T<Byte*>(malloc(buffer_size));

	memset(buffer, buffer_size, 0);

	PMap* map_ = T2T<PMap*>(buffer);

	map_->InitByBlock(buffer_size - sizeof(PMap));

	FillPMap(map_, map_->max_size());


	Int operations = 1000*1000;

	Int* rd_array_ = new Int[operations];
	for (Int c = 0; c < operations; c++)
	{
		rd_array_[c] = GetRandom(map_->size());
	}

	BigInt t0 = GetTimeInMillis();

	for (Int d = 0; d < average; d++)
	{
		for (Int c = 0; c < operations; c++)
		{
			BigInt key = rd_array_[c];
			if (map_->FindEQ(0, key) != key)
			{
				// this shouldn't happen
				cout<<"MISS! "<<key<<endl;
			}
		}
	}

	BigInt t1 = GetTimeInMillis();

	BigInt duration = (t1 / average - t0 / average);
	BigInt speed	= operations*1000 / duration;

	cout<<size<<" "<<speed<<endl;
}


int main(int argc, const char** argv, const char** envp)
{

	for (Int c = 1; c <= 128*1024; c*= 2) {
		Benchmark(c);
	}


	return 0;
}
