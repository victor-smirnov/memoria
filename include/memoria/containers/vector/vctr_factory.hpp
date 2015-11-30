
// Copyright Victor Smirnov 2013+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_VCTR_FACTORY_HPP
#define _MEMORIA_CONTAINERS_VCTR_FACTORY_HPP

#include <memoria/prototypes/bt_ss/btss_factory.hpp>

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
struct BTTypes<Profile, memoria::Vector<Value_> >: public BTTypes<Profile, memoria::BTSingleStream> {

    typedef BTTypes<Profile, memoria::BTSingleStream>                           Base;

    typedef Value_                                                              Value;

    using VectorStreamTF = StreamTF<
        TL<TL<PackedFSEArray<PackedFSEArrayTypes<Value>>>>,
        TL<TL<TL<>>>,
		FSEBranchStructTF
    >;


    typedef TypeList<VectorStreamTF>                                             StreamDescriptors;


    typedef BalancedTreeMetadata<
                typename Base::ID,
                ListSize<StreamDescriptors>::Value
    >                                                                           Metadata;


    using CommonContainerPartsList = MergeLists<
            typename Base::CommonContainerPartsList,

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
};



template <Granularity Gr> struct CodecClassTF;

template <>
struct CodecClassTF<Granularity::Byte> {
	template <typename V>
	using Type = UByteExintCodec<V>;
};


template <>
struct CodecClassTF<Granularity::Bit> {
	template <typename V>
	using Type = UBigIntEliasCodec<V>;
};


template <typename Profile, Granularity Gr, typename Value_>
struct BTTypes<Profile, memoria::Vector<VLen<Gr, Value_>> >: public BTTypes<Profile, memoria::BTSingleStream> {

    typedef BTTypes<Profile, memoria::BTSingleStream>                           Base;

    typedef Value_                                                              Value;

    using VectorStreamTF = StreamTF<
        TL<TL<
			PkdVDArrayT<Value, 1, CodecClassTF<Gr>::template Type>
    	>>,
        TL<TL<TL<>>>,
		FSEBranchStructTF
    >;


    typedef TypeList<
                VectorStreamTF
    >                                                                           StreamDescriptors;

    typedef BalancedTreeMetadata<
                typename Base::ID,
                ListSize<StreamDescriptors>::Value
    >                                                                           Metadata;


    using CommonContainerPartsList = MergeLists<
            typename Base::CommonContainerPartsList,

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
};








template <typename Profile, typename Value, typename T>
class CtrTF<Profile, memoria::Vector<Value>, T>: public CtrTF<Profile, memoria::BTSingleStream, T> {

    using Base = CtrTF<Profile, memoria::BTSingleStream, T>;
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
