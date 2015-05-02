
// Copyright Victor Smirnov 2013-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_VCTR_FACTORY_HPP
#define _MEMORIA_CONTAINERS_VCTR_FACTORY_HPP

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

    struct StreamTF {
        typedef BigInt                                              Key;
        typedef Value_                                              Value;

        typedef core::StaticVector<BigInt, 1>                       AccumulatorPart;
        typedef core::StaticVector<BigInt, 1>                       IteratorPrefixPart;

        typedef PkdVTree<Packed2TreeTypes<Key, Key, 1, UByteExintCodec>>             NonLeafType;
        typedef TL<PackedFSEArray<PackedFSEArrayTypes<Value>>>      LeafType;
        typedef TL<TL<>>											IdxRangeList;
    };


    typedef TypeList<
                StreamTF
    >                                                                           StreamDescriptors;

    typedef BalancedTreeMetadata<
                typename Base::ID,
                ListSize<StreamDescriptors>::Value
    >                                                                           Metadata;


    using ContainerPartsList = MergeLists<
            typename Base::ContainerPartsList,
            bt::NodeComprName,
            bt::InsertBatchComprName,

            mvector::CtrToolsName,
            mvector::CtrInsertName,
            mvector::CtrRemoveName,
            mvector::CtrFindName,
            mvector::CtrApiName
    >;

    using IteratorPartsList = MergeLists<
            typename Base::IteratorPartsList,
            mvector::ItrApiName
    >;

    typedef IDataSource<Value>                                                  DataSource;
    typedef IDataTarget<Value>                                                  DataTarget;

    typedef std::tuple<DataSource*>                                             Source;


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
