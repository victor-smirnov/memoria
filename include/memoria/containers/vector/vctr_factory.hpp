
// Copyright Victor Smirnov 2013-2014.
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

    typedef BTTypes<Profile, memoria::BT>                                       Base;

    typedef Value_                                                              Value;


    template <typename Iterator, typename Container>
    struct IteratorCacheFactory {
        typedef ::memoria::mvector::VectorIteratorPrefixCache<Iterator, Container>               Type;
    };

//    typedef TypeList<
//            LeafNodeTypes<LeafNode>,
//            NonLeafNodeTypes<BranchNode>
//    >                                                                           NodeTypesList;
//
//    typedef TypeList<
//            TreeNodeType<LeafNode>,
//            TreeNodeType<BranchNode>
//    >                                                                           DefaultNodeTypesList;


    struct StreamTF {
        typedef BigInt                                              Key;
        typedef Value_                                              Value;

        typedef core::StaticVector<BigInt, 1>                       AccumulatorPart;
        typedef core::StaticVector<BigInt, 1>                       IteratorPrefixPart;

        typedef PkdFTree<Packed2TreeTypes<Key, Key, 1>>             NonLeafType;
        typedef PackedFSEArray<PackedFSEArrayTypes<Value>>          LeafType;
    };


    typedef TypeList<
                StreamTF
    >                                                                           StreamDescriptors;

    typedef BalancedTreeMetadata<
                typename Base::ID,
                ListSize<StreamDescriptors>::Value
    >                                                                           Metadata;


    typedef typename MergeLists<
            typename Base::ContainerPartsList,
            bt::NodeNormName,

            mvector::CtrToolsName,
            mvector::CtrInsertName,
            mvector::CtrRemoveName,
            mvector::CtrFindName,
            mvector::CtrApiName
    >::Result                                                                   ContainerPartsList;

    typedef typename MergeLists<
            typename Base::IteratorPartsList,
            mvector::ItrApiName
    >::Result                                                                   IteratorPartsList;

    typedef IDataSource<Value>                                                  DataSource;
    typedef IDataTarget<Value>                                                  DataTarget;

    typedef std::tuple<DataSource*>												Source;



    template <typename Types>
    using FindGTWalker          = SkipForwardWalker<Types, 0>;

    template <typename Types>
    using FindGEWalker          = ::memoria::mvector::FindGEWalker<Types>;


    template <typename Types>
    using SkipForwardWalker     = SkipForwardWalker<Types, 0>;

    template <typename Types>
    using SkipBackwardWalker    = SkipBackwardWalker<Types, 0>;

    template <typename Types>
    using NextLeafWalker        = NextLeafWalker<Types, 0>;

    template <typename Types>
    using PrevLeafWalker        = PrevLeafWalker<Types, 0>;



    template <typename Types>
    using FindBeginWalker       = ::memoria::mvector::FindBeginWalker<Types>;
};


template <typename Profile, typename Value, typename T>
class CtrTF<Profile, memoria::Vector<Value>, T>: public CtrTF<Profile, memoria::BT, T> {

    using Base = CtrTF<Profile, memoria::BT, T>;
public:

    struct Types: Base::Types
    {
        typedef Vector2CtrTypes<Types>                                          CtrTypes;
        typedef Vector2IterTypes<Types>                                         IterTypes;

        typedef PageUpdateManager<CtrTypes>                                     PageUpdateMgr;
    };


    typedef typename Types::CtrTypes                                            CtrTypes;
    typedef Ctr<CtrTypes>                                                       Type;

};




}

#endif
