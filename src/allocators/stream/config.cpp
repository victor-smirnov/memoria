
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)





#include <memoria/allocators/stream/factory.hpp>

#include <iostream>

using namespace std;
using namespace memoria;


template class ContainerTypesCollection<StreamProfile<> >;
template class ::memoria::StreamAllocator<StreamProfile<>, BasicContainerCollectionCfg<StreamProfile<> >::Page, EmptyType>;
template class Checker<StreamContainerTypesCollection, DefaultStreamAllocator>;


typedef StreamContainerTypesCollection::Factory<DefKVMap>::Type KVMapType;


void CheckAllocatorConcept() {
	DefaultStreamAllocator allocator;
	StreamContainersChecker checker(allocator);
	checker.CheckAll();
}

void KVMapConcept() {

	DefaultStreamAllocator allocator;

	KVMapType* map = new KVMapType(allocator, 1, true);

	map->Put(0,0);

	map->GetSize();

	typedef KVMapType::Iterator IteratorType;

	IteratorType iter1 = map->Begin();
	IteratorType iter2 = iter1;

	iter1.SkipKeyFw(10);
	iter1.SkipKeyBw(10);
	iter2.NextKey();
	iter2.PrevKey();

	IteratorType iter3 = map->RBegin();
	IteratorType iter4 = map->REnd();

	IteratorType iter5 = map->End();

	if (iter1 != iter2 && iter3 == iter4 && iter4 == iter5)
	{
		cout<<"HERE"<<endl;
	}
}
