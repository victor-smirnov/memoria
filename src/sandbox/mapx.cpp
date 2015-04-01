// Copyright Victor Smirnov 2014-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#include <memoria/memoria.hpp>
#include <memoria/containers/mapx/mapx_factory.hpp>
#include <memoria/core/container/metadata_repository.hpp>


using namespace memoria;
using namespace std;

int main() {
	MEMORIA_INIT(SmallProfile<>);

	SmallInMemAllocator alloc;

	using CtrT = SCtrTF<MapX<BigInt, BigInt>>::Type;

	CtrT::initMetadata();

	CtrT ctr(&alloc);

	using LinearLeafList = FlattenLeafTree<CtrT::Types::LeafStreamsStructList>;

	TypesPrinter<
		LeafOffsetListBuilder<CtrT::Types::LeafStreamsStructList>::Type,
//		CtrT::Types::IteratorAccumulator,
		CtrT::Types::Accumulator,
		LinearLeafList,
		CtrT::Types::BranchStreamsStructList
	>::print(cout);

}
