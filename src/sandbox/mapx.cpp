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

	try {
		SmallInMemAllocator alloc;

		using RootT = SCtrTF<Root>::Type;
		using CtrT = SCtrTF<MapX<BigInt, BigInt>>::Type;

		CtrT::initMetadata();

		CtrT ctr(&alloc);

		auto iter = ctr.Begin();

		ctr.template insertStreamEntry<0>(iter, std::make_tuple(core::StaticVector<BigInt, 1>({1}), 5));
		ctr.template insertStreamEntry<0>(iter, std::make_tuple(core::StaticVector<BigInt, 1>({1}), 6));
		ctr.template insertStreamEntry<0>(iter, std::make_tuple(core::StaticVector<BigInt, 1>({1}), 7));
		ctr.template insertStreamEntry<0>(iter, std::make_tuple(core::StaticVector<BigInt, 1>({1}), 8));

		iter = ctr.findK(2);

		iter.dump();

//		using LinearLeafList = FlattenLeafTree<CtrT::Types::LeafStreamsStructList>;
//
//		TypesPrinter<
//		IntValue<CtrT::Types::Streams>
//		RootT::Types::LeafRangeOffsetList,
//		CtrT::Types::LeafRangeOffsetList//,
//
//		LeafOffsetListBuilder<CtrT::Types::LeafStreamsStructList>::Type,
//		CtrT::Types::IteratorAccumulator,
//		CtrT::Types::Accumulator,
//		LinearLeafList,
//		CtrT::Types::BranchStreamsStructList
//		>::print(cout);
	}
	catch (memoria::vapi::Exception& ex) {
		cout<<ex.message()<<" at "<<ex.source()<<endl;
	}
}
