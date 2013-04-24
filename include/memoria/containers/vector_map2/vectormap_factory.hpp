
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_VECTORMAP2_FACTORY_HPP
#define _MEMORIA_CONTAINERS_VECTORMAP2_FACTORY_HPP

#include <memoria/containers/map2/map_factory.hpp>
#include <memoria/containers/vector2/vector_factory.hpp>

#include <memoria/containers/vector_map2/vectormap_walkers.hpp>
#include <memoria/containers/vector_map2/vectormap_tools.hpp>
#include <memoria/containers/vector_map2/vectormap_names.hpp>

#include <memoria/containers/vector_map2/container/vectormap_c_checks.hpp>
#include <memoria/containers/vector_map2/container/vectormap_c_tools.hpp>
#include <memoria/containers/vector_map2/container/vectormap_c_insert.hpp>
#include <memoria/containers/vector_map2/container/vectormap_c_remove.hpp>
#include <memoria/containers/vector_map2/container/vectormap_c_api.hpp>
#include <memoria/containers/vector_map2/container/vectormap_c_find.hpp>

#include <memoria/containers/vector_map2/vectormap_iterator.hpp>
#include <memoria/containers/vector_map2/iterator/vectormap_i_api.hpp>

#include <memoria/containers/vector_map2/vectormap_names.hpp>

namespace memoria    {



template <typename Profile, typename Key_, typename Value_>
struct BalancedTreeTypes<Profile, memoria::VectorMap<Key_, Value_> >:
	public BalancedTreeTypes<Profile, memoria::BalancedTree>
{

    typedef BalancedTreeTypes<Profile, memoria::BalancedTree>                   Base;

    typedef Value_                                                          	Value;
    typedef TypeList<BigInt>                                                  	KeysList;

    static const Int Indexes                                                	= 1;


    template <typename Iterator, typename Container>
    struct IteratorCacheFactory {
        typedef vmap::VectorMapIteratorPrefixCache<Iterator, Container>         Type;
    };

    typedef TypeList<
    		NonLeafNodeTypes<TreeMapNode>,
    		LeafNodeTypes<TreeLeafNode>
    >																			NodeTypesList;

    typedef TypeList<
        		LeafNodeType<TreeLeafNode>,
        		InternalNodeType<TreeMapNode>,
        		RootNodeType<TreeMapNode>,
        		RootLeafNodeType<TreeLeafNode>
    >																			DefaultNodeTypesList;

    typedef TypeList<
        		// Vector
    			StreamDescr<
        			PackedFSETreeTF,
        			PackedFSEArrayTF,
        			1
        		>,
        		// Map
        		StreamDescr<
        			PackedFSETreeTF,
        			PackedFSETreeTF,
        			2
        		>
    >																			StreamDescriptors;


    typedef typename MergeLists<
    		typename Base::ContainerPartsList,
    		memoria::vmap::CtrToolsName,
    		memoria::vmap::CtrInsertName,
    		memoria::vmap::CtrRemoveName,
    		memoria::vmap::CtrChecksName,
    		memoria::vmap::CtrFindName,
    		memoria::vmap::CtrApiName
    >::Result                                           						ContainerPartsList;

    typedef typename MergeLists<
    		typename Base::IteratorPartsList,
    		memoria::vmap::ItrApiName
    >::Result                                           						IteratorPartsList;

    typedef IDataSource<Value>													DataSource;
    typedef IDataTarget<Value>													DataTarget;



    template <typename Types>
    using FindLTWalker 		= SkipForwardWalker<Types>;

    template <typename Types>
    using FindLEWalker 		= ::memoria::mvector2::FindLEWalker<Types>;



    template <typename Types>
    using FindBeginWalker 	= ::memoria::map::FindBeginWalker<Types>;

    template <typename Types>
    using FindEndWalker 	= ::memoria::map::FindEndWalker<Types>;

    template <typename Types>
    using FindRBeginWalker 	= ::memoria::map::FindRBeginWalker<Types>;

    template <typename Types>
    using FindREndWalker 	= ::memoria::map::FindREndWalker<Types>;
};


template <typename Profile, typename Key, typename Value, typename T>
class CtrTF<Profile, memoria::VectorMap<Key, Value>, T>: public CtrTF<Profile, memoria::BalancedTree, T> {
};




}

#endif
