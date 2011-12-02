
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include "tools.hpp"
#include "mtools.hpp"

#include <memoria/allocators/stream/factory.hpp>
#include <memoria/core/tools/bm_tools.hpp>

#include <iostream>
#include <vector>
#include <set>
#include <algorithm>

using namespace memoria;
using namespace memoria::vapi;


using namespace std;

const Int SIZE = 4000;

const BigInt MAX_VALUE = 0x7fffffffffffffff;

typedef StreamContainerTypesCollection::Factory<DefKVMap>::Type KVMapType;
typedef Ctr<StreamContainerTypesCollection::Types<memoria::IdxMap>::Type::CtrTypes> IdxMapType;

void checkIterator(IdxMapType* map, IDPairVector& pairs)
{
	if (pairs.size() > 0)
	{
		Int idx = 0;
		for (auto iter = map->Begin(); !iter.IsEnd(); iter.Next(), idx++)
		{
			BigInt  key 	= iter.GetKey(0);
			if (pairs[idx].key_ != key) cout<<"fw key "<<idx<<endl;
		}

		if (idx != (int)pairs.size()) cout<<"fw size "<<idx<<" "<<pairs.size()<<endl;

		idx = (Int)pairs.size() - 1;
		for (auto iter = map->RBegin(); !iter.IsStart(); iter.Prev(), idx--)
		{
			BigInt  key 	= iter.GetKey(0);
			if (pairs[idx].key_ != key) cout<<"bw key "<<idx<<" "<<key<<" "<<pairs[idx].key_<<endl;
		}

		if (idx != -1) cout<<"bw size "<<idx<<" "<<pairs.size()<<endl;
	}
}

set<long long> randoms;

long long getLongRandom() {
	long long rnd = 0;
	int cnt = 0;
	while (randoms.count(rnd = get_random(10000)) > 0)
	{
		cnt++;
		if (cnt == 10000) throw "can't create random value";
	}
	randoms.insert(rnd);
	return rnd;
}



int main(int argc, const char **argv, const char** envp)
{
	long long t0 = getTime();

	try {
		InitTypeSystem(argc, argv, envp, false);

		logger.level() = Logger::NONE;

		StreamContainerTypesCollection::Init();

		IDPairVector pairs;
		IDPairVector pairs_sorted;

		for (Int c = 0; c < SIZE; c++)
		{
			KIDPair pair(getLongRandom(), IDValue());

			pairs.push_back(pair);
			pairs_sorted.push_back(pair);
		}

		sort(pairs_sorted);

		for (UInt x = 0; x < 1000; x++)
		{
//			cout<<"x="<<x<<endl;
			DefaultStreamAllocator allocator;

			IdxMapType* map = new IdxMapType(allocator, 1, true);

//			map->SetMaxChildrenPerNode(5);

			for (int c = 0; c < SIZE; c++)
			{
				map->Put(pairs[c].key_, 0);
			}

			UInt from, to;
			if (x == 0)
			{
				from 	= 0;
				to 		= SIZE - 1;
			}
			else if (x == 1)
			{
				from 	= 0;
				to 		= SIZE/2;
			}
			else if (x == 2)
			{
				from 	= SIZE/2;
				to 		= SIZE - 1;
			}
			else {
				from 	= get_random(SIZE/2 - 1);
				to 		= get_random(SIZE/2) + SIZE/2;
			}

			IDPairVector result;
			for (UInt d = 0; d < pairs_sorted.size(); d++)
			{
				if (d < from || d > to) result.push_back(pairs_sorted[d]);
			}

			BigInt from_key = pairs_sorted[from].key_;
			BigInt to_key   = pairs_sorted[to].key_ + 1;

			map->Remove(from_key, to_key);

//			cout<<"Size after remove: "<<map->GetSize()<<endl;

			long size = SIZE - (to - from + 1);
			if (size != map->GetSize()) cout<<"Remove.size "<<x<<endl;

			if (check(allocator))
			{
				break;
			}

			checkIterator(map, result);
		}
	}
	catch (MemoriaException ex) {
		cout<<"MemoriaException "<<ex.source()<<" "<<ex.message()<<endl;
	}
	catch (MemoriaException *ex) {
		cout<<"MemoriaException* "<<ex->source()<<" "<<ex->message()<<endl;
	}
	catch (int i) {
		cout<<"IntegerEx: "<<i<<endl;
	}
	catch (exception e) {
		cout<<"StdEx: "<<e.what()<<endl;
	}
	catch(...) {
		cout<<"Unrecognized exception"<<endl;
	}

	cout<<"IDX MAP time: "<<(getTime()- t0)<<endl;

	Int CtrTotal = 0, DtrTotal = 0;
	for (Int c = 0; c < (Int)(sizeof(PageCtrCnt)/sizeof(Int)); c++)
	{
		cout<<c<<" "<<PageCtrCnt[c]<<" "<<PageDtrCnt[c]<<" "<<(PageCtrCnt[c] + PageDtrCnt[c])<<endl;
		CtrTotal += PageCtrCnt[c];
		DtrTotal += PageDtrCnt[c];
	}

	cout<<"Total: "<<CtrTotal<<" "<<DtrTotal<<" "<<(CtrTotal + DtrTotal)<<endl;
	cout<<"Total: "<<PageCtr<<" "<<PageDtr<<" "<<(PageCtr + PageDtr)<<endl;
}
