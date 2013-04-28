
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_VECTOR2_FACTORY_HPP
#define _MEMORIA_CONTAINERS_VECTOR2_FACTORY_HPP

#include <memoria/prototypes/balanced_tree/balanced_tree.hpp>

#include <memoria/core/packed2/packed_fse_array.hpp>

#include <memoria/containers/vector2/vector_walkers.hpp>
#include <memoria/containers/vector2/vector_tools.hpp>
#include <memoria/containers/vector2/vector_names.hpp>

#include <memoria/containers/vector2/container/vector_c_checks.hpp>
#include <memoria/containers/vector2/container/vector_c_tools.hpp>
#include <memoria/containers/vector2/container/vector_c_insert.hpp>
#include <memoria/containers/vector2/container/vector_c_remove.hpp>
#include <memoria/containers/vector2/container/vector_c_api.hpp>
#include <memoria/containers/vector2/container/vector_c_find.hpp>

#include <memoria/containers/vector2/vector_iterator.hpp>
#include <memoria/containers/vector2/iterator/vector_i_api.hpp>

#include <memoria/containers/vector2/vector_names.hpp>

namespace memoria    {



template <typename Profile, typename Value_>
struct BalancedTreeTypes<Profile, memoria::Vector<Value_> >: public BalancedTreeTypes<Profile, memoria::BalancedTree> {

    typedef BalancedTreeTypes<Profile, memoria::BalancedTree>                   Base;

    typedef Value_                                                          	Value;
    typedef TypeList<BigInt>                                                  	KeysList;

    static const Int Indexes                                                	= 1;


    template <typename Iterator, typename Container>
    struct IteratorCacheFactory {
        typedef ::memoria::mvector2::VectorIteratorPrefixCache<Iterator, Container>               Type;
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
        		StreamDescr<
        			PackedFSETreeTF,
        			PackedFSEArrayTF,
        			1
        	>
    >																			StreamDescriptors;

    typedef BalancedTreeMetadata<
        		typename Base::ID,
        		ListSize<StreamDescriptors>::Value
    >        																	Metadata;


    typedef typename MergeLists<
    		typename Base::ContainerPartsList,
    		memoria::mvector2::CtrToolsName,
    		memoria::mvector2::CtrInsertName,
    		memoria::mvector2::CtrRemoveName,
    		memoria::mvector2::CtrChecksName,
    		memoria::mvector2::CtrFindName,
    		memoria::mvector2::CtrApiName
    >::Result                                           						ContainerPartsList;

    typedef typename MergeLists<
    		typename Base::IteratorPartsList,
    		memoria::mvector2::ItrApiName
    >::Result                                           						IteratorPartsList;

    typedef IDataSource<Value>													DataSource;
    typedef IDataTarget<Value>													DataTarget;



    template <typename Types>
    using FindLTWalker 			= SkipForwardWalker<Types>;

    template <typename Types>
    using FindLEWalker 			= ::memoria::mvector2::FindLEWalker<Types>;


    template <typename Types>
    using SkipForwardWalker 	= SkipForwardWalker<Types>;

    template <typename Types>
    using SkipBackwardWalker 	= SkipBackwardWalker<Types>;

    template <typename Types>
    using NextLeafWalker 	 	= NextLeafWalker<Types>;

    template <typename Types>
    using PrevLeafWalker 		= PrevLeafWalker<Types>;



    template <typename Types>
    using FindBeginWalker 		= ::memoria::mvector2::FindBeginWalker<Types>;

    template <typename Types>
    using FindEndWalker 		= ::memoria::mvector2::FindEndWalker<Types>;

    template <typename Types>
    using FindRBeginWalker 		= ::memoria::mvector2::FindRBeginWalker<Types>;

    template <typename Types>
    using FindREndWalker 		= ::memoria::mvector2::FindREndWalker<Types>;
};


template <typename Profile, typename Value, typename T>
class CtrTF<Profile, memoria::Vector<Value>, T>: public CtrTF<Profile, memoria::BalancedTree, T> {
};




}

#endif
