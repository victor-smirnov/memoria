
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_vctr_FACTORY_HPP
#define _MEMORIA_CONTAINERS_vctr_FACTORY_HPP

#include <memoria/prototypes/bt/bt_factory.hpp>

#include <memoria/core/packed/array/packed_fse_array.hpp>

#include <memoria/containers/vector/vctr_walkers.hpp>
#include <memoria/containers/vector/vctr_tools.hpp>
#include <memoria/containers/vector/vctr_names.hpp>

#include <memoria/containers/vector/container/vctr_c_checks.hpp>
#include <memoria/containers/vector/container/vctr_c_tools.hpp>
#include <memoria/containers/vector/container/vctr_c_insert.hpp>
#include <memoria/containers/vector/container/vctr_c_remove.hpp>
#include <memoria/containers/vector/container/vctr_c_api.hpp>
#include <memoria/containers/vector/container/vctr_c_find.hpp>

#include <memoria/containers/vector/vctr_iterator.hpp>
#include <memoria/containers/vector/iterator/vctr_i_api.hpp>

#include <memoria/containers/vector/vctr_names.hpp>

namespace memoria    {



template <typename Profile, typename Value_>
struct BTTypes<Profile, memoria::Vector<Value_> >: public BTTypes<Profile, memoria::BT> {

    typedef BTTypes<Profile, memoria::BT>                   Base;

    typedef Value_                                                          	Value;
    typedef TypeList<BigInt>                                                  	KeysList;

    static const Int Indexes                                                	= 1;


    template <typename Iterator, typename Container>
    struct IteratorCacheFactory {
        typedef ::memoria::mvector::VectorIteratorPrefixCache<Iterator, Container>               Type;
    };

    typedef TypeList<
    		LeafNodeTypes<LeafNode>,
    		NonLeafNodeTypes<BranchNode>
    >																			NodeTypesList;

    typedef TypeList<
        		LeafNodeType<LeafNode>,
        		BranchNodeType<BranchNode>
    >																			DefaultNodeTypesList;

    typedef TypeList<
        		StreamDescr<
        			PkdFTreeTF,
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
    		bt::NodeNormName,
    		mvector::CtrToolsName,
    		mvector::CtrInsertName,
    		mvector::CtrRemoveName,
    		mvector::CtrChecksName,
    		mvector::CtrFindName,
    		mvector::CtrApiName
    >::Result                                           						ContainerPartsList;

    typedef typename MergeLists<
    		typename Base::IteratorPartsList,
    		mvector::ItrApiName
    >::Result                                           						IteratorPartsList;

    typedef IDataSource<Value>													DataSource;
    typedef IDataTarget<Value>													DataTarget;



    template <typename Types>
    using FindLTWalker 			= SkipForwardWalker<Types>;

    template <typename Types>
    using FindLEWalker 			= ::memoria::mvector::FindLEWalker<Types>;


    template <typename Types>
    using SkipForwardWalker 	= SkipForwardWalker<Types>;

    template <typename Types>
    using SkipBackwardWalker 	= SkipBackwardWalker<Types>;

    template <typename Types>
    using NextLeafWalker 	 	= NextLeafWalker<Types>;

    template <typename Types>
    using PrevLeafWalker 		= PrevLeafWalker<Types>;



    template <typename Types>
    using FindBeginWalker 		= ::memoria::mvector::FindBeginWalker<Types>;

    template <typename Types>
    using FindEndWalker 		= ::memoria::mvector::FindEndWalker<Types>;

    template <typename Types>
    using FindRBeginWalker 		= ::memoria::mvector::FindRBeginWalker<Types>;

    template <typename Types>
    using FindREndWalker 		= ::memoria::mvector::FindREndWalker<Types>;
};


template <typename Profile, typename Value, typename T>
class CtrTF<Profile, memoria::Vector<Value>, T>: public CtrTF<Profile, memoria::BT, T> {
};




}

#endif
