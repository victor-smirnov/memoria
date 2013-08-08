
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_WT_FACTORY_HPP
#define _MEMORIA_CONTAINERS_WT_FACTORY_HPP

#include <memoria/containers/seq_dense/seqd_factory.hpp>
#include <memoria/containers/labeled_tree/ltree_factory.hpp>

#include <memoria/containers/wt/wt_walkers.hpp>
#include <memoria/containers/wt/wt_tools.hpp>
#include <memoria/containers/wt/wt_names.hpp>

//#include <memoria/containers/vector_map/container/vmap_c_checks.hpp>
//#include <memoria/containers/vector_map/container/vmap_c_tools.hpp>
//#include <memoria/containers/vector_map/container/vmap_c_insert.hpp>
//#include <memoria/containers/vector_map/container/vmap_c_remove.hpp>
//#include <memoria/containers/vector_map/container/vmap_c_api.hpp>
//#include <memoria/containers/vector_map/container/vmap_c_find.hpp>
//#include <memoria/containers/vector_map/container/vmap_c_update.hpp>
//
//#include <memoria/containers/vector_map/vmap_iterator.hpp>
//#include <memoria/containers/vector_map/iterator/vmap_i_api.hpp>



namespace memoria    {

//
//template <typename Types, Int StreamIdx>
//struct PackedVMapFSETreeLeafTF {
//
//    typedef typename Types::Key                                                 Key;
//
//    typedef typename SelectByIndexTool<
//    		StreamIdx,
//    		typename Types::StreamDescriptors
//    >::Result																	Descriptor;
//
//	typedef Packed2TreeTypes<
//			Key, Key, Descriptor::LeafIndexes
//	>																			TreeTypes;
//
//	typedef PkdFTree<TreeTypes> Type;
//};
//
//
//
//template <typename Profile, typename Key_, typename Value_>
//struct BTTypes<Profile, memoria::VectorMap<Key_, Value_> >:
//	public BTTypes<Profile, memoria::BT>
//{
//
//    typedef BTTypes<Profile, memoria::BT>                   Base;
//
//    typedef Value_                                                          	Value;
//    typedef TypeList<BigInt>                                                  	KeysList;
//
//    static const Int Indexes                                                	= 1;
//
//
//    template <typename Iterator, typename Container>
//    struct IteratorCacheFactory {
//        typedef vmap::VectorMapIteratorPrefixCache<Iterator, Container>         Type;
//    };
//
//    typedef TypeList<
//    		NonLeafNodeTypes<TreeMapNode>,
//    		LeafNodeTypes<TreeLeafNode>
//    >																			NodeTypesList;
//
//    typedef TypeList<
//        		LeafNodeType<TreeLeafNode>,
//        		InternalNodeType<TreeMapNode>,
//        		RootNodeType<TreeMapNode>,
//        		RootLeafNodeType<TreeLeafNode>
//    >																			DefaultNodeTypesList;
//
//    typedef TypeList<
//    			// Map
//    			StreamDescr<
//    				PkdFTreeTF,
//    				PackedVMapFSETreeLeafTF,
//    				2
//    			>,
//
//    			// Vector
//    			StreamDescr<
//        			PkdFTreeTF,
//        			PackedFSEArrayTF,
//        			1
//        		>
//    >																			StreamDescriptors;
//
//    typedef BalancedTreeMetadata<
//        		typename Base::ID,
//        		ListSize<StreamDescriptors>::Value
//    > 																			Metadata;
//
//
//    typedef typename MergeLists<
//    		typename Base::ContainerPartsList,
//    		memoria::bt::NodeNormName,
//    		memoria::vmap::CtrToolsName,
//    		memoria::vmap::CtrInsertName,
//    		memoria::vmap::CtrRemoveName,
//    		memoria::vmap::CtrUpdateName,
//    		memoria::vmap::CtrChecksName,
//    		memoria::vmap::CtrFindName,
//    		memoria::vmap::CtrApiName
//    >::Result                                           						ContainerPartsList;
//
//    typedef typename MergeLists<
//    		typename Base::IteratorPartsList,
//    		memoria::vmap::ItrApiName
//    >::Result                                           						IteratorPartsList;
//
//    typedef IDataSource<Value>													DataSource;
//    typedef IDataTarget<Value>													DataTarget;
//
//
//
////    template <typename Types>
////    using FindLTWalker 		= ::memoria::vmap::FindLTForwardWalker<Types>;
////
////    template <typename Types>
////    using FindLEWalker 		= ::memoria::vmap::FindLTForwardWalker<Types>;
//
//    template <typename Types>
//    using SkipForwardWalker = ::memoria::vmap::SkipForwardWalker<Types>;
//
//    template <typename Types>
//    using SkipBackwardWalker = TypeIsNotDefined;
//
//
//    template <typename Types>
//    using NextLeafWalker = NextLeafWalker<Types>;
//
//    template <typename Types>
//    using PrevLeafWalker = ::memoria::vmap::PrevLeafWalker<Types>;
//
//    template <typename Types>
//    using NextLeafMutistreamWalker = NextLeafMultistreamWalker<Types>;
//
//
//    template <typename Types>
//    using PrevLeafMutistreamWalker = PrevLeafMultistreamWalker<Types>;
//
//
//
//    template <typename Types>
//    using FindBeginWalker 	= ::memoria::vmap::FindVMapBeginWalker<Types>;
//
//    template <typename Types>
//    using FindEndWalker 	= ::memoria::vmap::FindVMapEndWalker<Types>;
//
//    template <typename Types>
//    using FindRBeginWalker 	= ::memoria::vmap::FindVMapRBeginWalker<Types>;
//
//    template <typename Types>
//    using FindREndWalker 	= ::memoria::vmap::FindVMapREndWalker<Types>;
//};
//
//
//template <typename Profile, typename Key, typename Value, typename T>
//class CtrTF<Profile, memoria::VectorMap<Key, Value>, T>: public CtrTF<Profile, memoria::BT, T> {
//};




}

#endif
