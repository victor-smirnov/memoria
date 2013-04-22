
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_MODELS_IDX_MAP2_FACTORY_HPP
#define _MEMORIA_MODELS_IDX_MAP2_FACTORY_HPP

#include <memoria/prototypes/balanced_tree/balanced_tree.hpp>
#include <memoria/prototypes/ctr_wrapper/ctrwrapper_factory.hpp>

#include <memoria/containers/map2/map_walkers.hpp>
#include <memoria/containers/map2/map_tools.hpp>

#include <memoria/containers/map2/container/map_c_tools.hpp>
#include <memoria/containers/map2/container/map_c_insert.hpp>
#include <memoria/containers/map2/container/map_c_remove.hpp>
#include <memoria/containers/map2/container/map_c_api.hpp>

#include <memoria/containers/map2/map_iterator.hpp>
#include <memoria/containers/map2/iterator/map_i_api.hpp>
#include <memoria/containers/map2/iterator/map_i_nav.hpp>




#include <memoria/containers/map2/map_names.hpp>

namespace memoria    {



template <typename Profile, typename Key_, typename Value_>
struct BalancedTreeTypes<Profile, memoria::Map2<Key_, Value_> >: public BalancedTreeTypes<Profile, memoria::BalancedTree> {

    typedef BalancedTreeTypes<Profile, memoria::BalancedTree>                   Base;

    typedef Value_                                                          	Value;
    typedef TypeList<Key_>                                                  	KeysList;

    static const Int Indexes                                                	= 1;


    typedef TypeList<
    		AllNodeTypes<balanced_tree::TreeMapNode>
    >																			NodeTypesList;

    typedef TypeList<
        		LeafNodeType<TreeMapNode>,
        		InternalNodeType<TreeMapNode>,
        		RootNodeType<TreeMapNode>,
        		RootLeafNodeType<TreeMapNode>
    >																			DefaultNodeTypesList;

    typedef TypeList<
        		StreamDescr<PackedFSETreeTF, PackedFSETreeTF, 1>
    >																			StreamDescriptors;


	typedef typename MergeLists<
				typename Base::ContainerPartsList,
				memoria::map2::CtrToolsName,
				memoria::map2::CtrInsertName,
				memoria::map2::CtrRemoveName,
				memoria::map2::CtrApiName
	>::Result                                           						ContainerPartsList;


	typedef typename MergeLists<
				typename Base::IteratorPartsList,
				memoria::map2::ItrApiName,
				memoria::map2::ItrNavName
	>::Result                                           						IteratorPartsList;


	template <typename Iterator, typename Container>
	struct IteratorCacheFactory {
		typedef map2::MapIteratorPrefixCache<Iterator, Container> Type;
	};



    template <typename Types>
    using FindLTWalker 		= ::memoria::map2::FindLTWalker<Types>;

    template <typename Types>
    using FindLEWalker 		= ::memoria::map2::FindLEWalker<Types>;



    template <typename Types>
    using FindBeginWalker 	= ::memoria::map2::FindBeginWalker<Types>;

    template <typename Types>
    using FindEndWalker 	= ::memoria::map2::FindEndWalker<Types>;

    template <typename Types>
    using FindRBeginWalker 	= ::memoria::map2::FindRBeginWalker<Types>;

    template <typename Types>
    using FindREndWalker 	= ::memoria::map2::FindREndWalker<Types>;
};



template <typename Profile, typename Key, typename Value, typename T>
class CtrTF<Profile, memoria::Map2<Key, Value>, T>: public CtrTF<Profile, memoria::BalancedTree, T> {

//	typedef CtrTF<Profile, memoria::BalancedTree, T> 							Base;
//
//public:
//
//    struct Types: Base::Types {
//    	typedef Map2CtrTypes<Types> 		CtrTypes;
//        typedef Map2IterTypes<Types> 		IterTypes;
//    };
//
//
//    typedef typename Types::CtrTypes                                            CtrTypes;
//    typedef typename Types::IterTypes                                           IterTypes;
//
//    typedef Ctr<CtrTypes>                                                       Type;
};




//template <typename Profile_, typename Key, typename Value>
//struct WrapperTypes<Profile_, CtrWrapper<memoria::Map2<Key, Value>>>: WrapperTypes<Profile_, memoria::Map2<Key, Value> > {
//
//	typedef WrapperTypes<Profile_, memoria::Map2<Key, Value>>   			Base;
//
//	typedef typename MergeLists<
//				typename Base::ContainerPartsList,
//				memoria::map2::CtrToolsName,
//				memoria::map2::CtrInsertName,
//				memoria::map2::CtrRemoveName,
//				memoria::map2::CtrApiName
//	>::Result                                           CtrList;
//
//	typedef typename MergeLists<
//				typename Base::IteratorPartsList,
//				memoria::map2::ItrApiName
//	>::Result                                           IterList;
//
//	typedef MapProto<Key, Value>           				WrappedCtrName;
//};



//template <typename Profile_, typename Key, typename Value, typename T>
//class CtrTF<Profile_, memoria::Map2<Key, Value>, T>: public CtrTF<Profile_, CtrWrapper<memoria::Map2<Key, Value>>, T> {
//
//	typedef CtrTF<Profile_, CtrWrapper<memoria::Map2<Key, Value>>, T> 								Base;
//
//public:
//
//    struct Types: Base::Types {
//    	typedef Map2CtrTypes<Types> 		CtrTypes;
//        typedef Map2IterTypes<Types> 		IterTypes;
//    };
//
//
//    typedef typename Types::CtrTypes                                            CtrTypes;
//    typedef typename Types::IterTypes                                           IterTypes;
//
//    typedef Ctr<CtrTypes>                                                       Type;
//};



}

#endif
