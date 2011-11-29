
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include "tools.hpp"
#include "mtools.hpp"

#include <memoria/allocators/stream/factory.hpp>
#include <memoria/core/tools/bm_tools.hpp>

using namespace memoria;
using namespace memoria::vapi;


using namespace std;

const Int SIZE = 10;

const BigInt MAX_BIGINT_VALUE = 0x7fffffffffffffff;
const Int MAX_INT_VALUE = 0x7fffffff;

typedef StreamContainerTypesCollection::Factory<DefKVMap>::Type KVMapType;

void checkIteratorFw(KVMapType* map, PairVector& pairs)
{
	typedef KVMapType::Iterator IteratorType;

	map->End().update();

	Int idx = 0;
	for (auto iter = map->Begin(); !iter.IsEnd(); )
	{
	    BigInt  key 	= iter.GetKey(0);
	    BigInt  value 	= iter.GetData();

	    if (pairs[idx].key_ != key) cout<<"key "<<idx<<" "<<pairs[idx].key_<<" "<<key<<endl;
	    if (pairs[idx].value_ != value) cout<<"value "<<idx<<" "<<pairs[idx].value_<<" "<<value<<endl;
	    
	    iter.Next();
	    idx++;
	}
	
	if (idx != (Int)pairs.size()) cout<<"fw iterator.size "<<idx<<" "<<pairs.size()<<endl;

	idx = (Int)pairs.size() - 1;
	for (auto iter = map->RBegin(); !iter.IsStart(); )
	{
		BigInt  key 	= iter.GetKey(0);
		BigInt  value 	= iter.GetData();

		if (pairs[idx].key_ != key) cout<<"key "<<idx<<" "<<pairs[idx].key_<<" "<<key<<endl;
		if (pairs[idx].value_ != value) cout<<"value "<<idx<<" "<<pairs[idx].value_<<" "<<value<<endl;

		iter.Prev();

		idx--;
	}

	if (idx != -1) cout<<"bw iterator.size "<<idx<<" "<<pairs.size()<<endl;
}


void checkIteratorBw(KVMapType* map, PairVector& pairs)
{
	typedef KVMapType::Iterator IteratorType;

//	IteratorType iter = map->RBegin();
//	IteratorType end = map->REnd();

	Int idx = pairs.size() - 1;
//	while (iter!=end)
	for (auto iter = map->RBegin(); !iter.IsStart(); )
	{
		BigInt  key 	= iter.GetKey(0);
		BigInt  value 	= iter.GetData();

	    if (pairs[idx].key_ != key) cout<<"key "<<idx<<" "<<pairs[idx].key_<<" "<<key<<endl;
	    if (pairs[idx].value_ != value) cout<<"value "<<idx<<" "<<pairs[idx].value_<<" "<<value<<endl;

	    iter.Prev();
	    idx--;
	}

	if (idx != -1) cout<<"iterator.size "<<idx<<" "<<pairs.size()<<endl;
}




void checkMultistepForwardIterator(KVMapType* map)
{
	typedef KVMapType::Iterator IteratorType;

	BigInt max = map->GetSize();

	for (Int c = 0; c < 100; c++)
	{
		IteratorType iter1 = map->Begin();
		IteratorType iter2 = iter1;

		BigInt rnd = max > 0 ? get_random(max) : 0;

		if (rnd > 0) iter1.SkipKeyFw(rnd);

		for (BigInt d = 0; d < rnd; d++)
		{
			iter2.NextKey();
		}

		if (iter1 != iter2)
		{
			cout<<"Multistep: "<<rnd<<" "<<iter1.key_idx()<<" "<<iter2.key_idx()<<endl;
		}
	}
}

void checkMultistepBackwardIterator(KVMapType* map)
{
	typedef KVMapType::Iterator IteratorType;

	BigInt max = map->GetSize();

	for (Int c = 0; c < 100; c++)
	{
		IteratorType iter1 = map->REnd();
		IteratorType iter2 = iter1;

		BigInt rnd = max > 0 ? get_random(max) : 0;

		if (rnd > 0) iter1.SkipKeyBw(rnd);

		for (BigInt d = 0; d < rnd; d++)
		{
			iter2.PrevKey();
		}

		if (iter1 != iter2)
		{
			cout<<"Multistep: "<<rnd<<" "<<iter1.key_idx()<<" "<<iter2.key_idx()<<endl;
		}
	}
}



int main(int argc, const char** argv, const char **envp) {

	long long t0 = getTime();

	try {
		InitTypeSystem(argc, argv, envp, false);

		logger.level() = Logger::NONE;

		ContainerTypesCollection<StreamProfile<> >::Init();
		StreamContainerTypesCollection::Init();

		DefaultStreamAllocator allocator;

		allocator.GetLogger()->level() = Logger::TRACE;

		PairVector pairs;
		PairVector pairs_sorted;

		for (Int c = 0; c < SIZE; c++)
		{
			pairs.push_back(KVPair(get_random(MAX_INT_VALUE), get_random(MAX_INT_VALUE)));
		}

		KVMapType* map = new KVMapType(allocator, 1, true);

//		map->SetMaxChildrenPerNode(5);

		for (int c = 0; c < SIZE; c++)
		{
			map->Put(pairs[c].key_, pairs[c].value_);
			map->GetSize();

			if(check(allocator))
			{
				throw MemoriaException("", "Check failed. c=" + ToString(c));
			}

			pairs_sorted.push_back(pairs[c]);
			sort(pairs_sorted);

			checkIteratorFw(map, pairs_sorted);
			checkIteratorBw(map, pairs_sorted);
			checkMultistepForwardIterator(map);
			checkMultistepBackwardIterator(map);
		}

		for (int c = 0; c < SIZE; c++)
		{
			BigInt value = 0;
			map->Get1(pairs[c].key_, value);

			if (pairs[c].value_ != value)
			{
				cout<<"query "<<c<<" "<<pairs[c].value_<<" "<<value<<endl;
			}
		}

		for (int c = 0; c < SIZE; c++)
		{
			map->Remove(pairs[c].key_);

			BigInt size = SIZE - c - 1;
			if (size != map->GetSize()) cout<<"Remove.size "<<c<<" "<<map->GetSize()<<endl;

			check(allocator);

			for (UInt x = 0; x < pairs_sorted.size(); x++)
			{
				if (pairs_sorted[x].key_ == pairs[c].key_)
				{
					pairs_sorted.erase(pairs_sorted.begin() + x);
				}
			}

			checkIteratorFw(map, pairs_sorted);

			if (c == SIZE - 1) {
				int a = 0;
				a++;
			}

			checkIteratorBw(map, pairs_sorted);
			if (c < SIZE - 1) {
				checkMultistepForwardIterator(map);
				checkMultistepBackwardIterator(map);
			}
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

	cout<<"TREE MAP time: "<<(getTime()- t0)<<endl;

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
