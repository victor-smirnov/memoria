
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_ROOT_FACTORY_HPP
#define _MEMORIA_MODELS_ROOT_FACTORY_HPP

//#include <memoria/prototypes/templates/map.hpp>
//#include <memoria/prototypes/templates/tree_map.hpp>
//
//#include <memoria/containers/kvmap/container/find.hpp>
//#include <memoria/containers/kvmap/container/checks.hpp>
//#include <memoria/containers/kvmap/container/insert.hpp>
//#include <memoria/containers/kvmap/container/remove.hpp>
//
//#include <memoria/prototypes/btree/btree.hpp>

#include <memoria/containers/root/container/container.hpp>


namespace memoria    {

using namespace memoria::btree;


//template <typename Profile>
//struct BTreeTypes<Profile, memoria::Root>: public BTreeTypes<Profile, memoria::BTree> {
//
//	typedef BTreeTypes<Profile, memoria::BTree> 								Base;
//	typedef memoria::btree::IDType 												Value;
//
//	typedef typename AppendTool<
//			typename Base::ContainerPartsList,
//			typename TLTool<
//			memoria::models::kvmap::MapApiName,
//			memoria::models::kvmap::FindName,
//			memoria::models::kvmap::ChecksName,
//			memoria::models::kvmap::InsertName,
//			memoria::models::kvmap::RemoveName,
//
//			memoria::models::TreeMapName,
//			memoria::models::MapName
//			>::List
//	>::Result                                                               	ContainerPartsList;
//};
//
//
//template <typename Profile, typename T>
//class CtrTF<Profile, memoria::Root, T>: public CtrTF<Profile, memoria::BTree, T> {};


template <typename Types>
struct RootCtrTypes: Types {};

template <typename Profile_, typename T>
class CtrTF<Profile_, memoria::Root, T> {

	typedef CtrTF<Profile_, memoria::Root, T> 									MyType;

	typedef typename ContainerCollectionCfg<Profile_>::Types::AbstractAllocator	Allocator;

public:

	struct Types {
		typedef Profile_					Profile;
		typedef MyType::Allocator			Allocator;

		typedef RootCtrTypes<Types>				CtrTypes;
	};

	typedef typename Types::CtrTypes 											CtrTypes;
	typedef Ctr<CtrTypes>														Type;
};


}

#endif
