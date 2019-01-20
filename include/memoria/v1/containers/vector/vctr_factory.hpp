
// Copyright 2013 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#pragma once

#include <memoria/v1/prototypes/bt_ss/btss_factory.hpp>

#include <memoria/v1/containers/vector/vctr_walkers.hpp>
#include <memoria/v1/containers/vector/vctr_tools.hpp>
#include <memoria/v1/containers/vector/vctr_names.hpp>

#include <memoria/v1/containers/vector/container/vctr_c_tools.hpp>
#include <memoria/v1/containers/vector/container/vctr_c_insert.hpp>
#include <memoria/v1/containers/vector/container/vctr_c_remove.hpp>
#include <memoria/v1/containers/vector/container/vctr_c_api.hpp>
#include <memoria/v1/containers/vector/container/vctr_c_find.hpp>

#include <memoria/v1/containers/vector/vctr_iterator.hpp>
#include <memoria/v1/containers/vector/iterator/vctr_i_api.hpp>

#include <memoria/v1/containers/vector/vctr_names.hpp>

#include <memoria/v1/core/packed/array/packed_fse_array.hpp>
#include <memoria/v1/core/packed/array/packed_vle_dense_array.hpp>
#include <memoria/v1/core/packed/misc/packed_sized_struct.hpp>

namespace memoria {
namespace v1 {

template <typename Profile, typename Value_>
struct VectorBTTypesBase: public BTTypes<Profile, BTSingleStream> {

    using Base = BTTypes<Profile, BTSingleStream>;

    using Value = Value_;
    using Entry = Value_;


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





template <typename Profile, typename Value>
struct BTTypes<Profile, Vector<Value> >: public VectorBTTypesBase<Profile, Value> {

    static_assert(
            IsExternalizable<Value>::Value ,
            "Value type must have either ValueCodec or FieldFactory defined"
    );


    using LeafValueStruct = typename mvector::VectorValueStructTF<Value, HasFieldFactory<Value>::Value>::Type;


    using StreamDescriptors = TL<bt::StreamTF<
        TL<
            StreamSize,
            LeafValueStruct
        >,
        bt::DefaultBranchStructTF,
        TL<TL<>, TL<>>
    >>;
};



template <Granularity Gr> struct CodecClassTF;

template <>
struct CodecClassTF<Granularity::int8_t> {
    template <typename V>
    using Type = UByteI7Codec<V>;
};


template <>
struct CodecClassTF<Granularity::Bit> {
    template <typename V>
    using Type = UInt64EliasCodec<V>;
};


template <typename Profile, Granularity Gr, typename Value_>
struct BTTypes<Profile, Vector<VLen<Gr, Value_>> >: public BTTypes<Profile, BTSingleStream> {

    typedef BTTypes<Profile, BTSingleStream>                           Base;

    typedef Value_                                                              Value;

    using VectorStreamTF = bt::StreamTF<
        TL<TL<
            StreamSize,
            PkdVDArrayT<Value, 1, CodecClassTF<Gr>::template Type>
        >>,
        bt::FSEBranchStructTF,
        TL<TL<TL<>, TL<>>>
    >;


    typedef TypeList<
        VectorStreamTF
    >                                                                           StreamDescriptors;

    using Entry = Value;

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
class CtrTF<Profile, Vector<Value>, T>: public CtrTF<Profile, BTSingleStream, T> {

    using Base = CtrTF<Profile, BTSingleStream, T>;
public:

    struct Types: Base::Types
    {
        typedef Vector2CtrTypes<Types>                                          CtrTypes;
        typedef Vector2IterTypes<Types>                                         IterTypes;

        typedef bt::BlockUpdateManager<CtrTypes>                                 BlockUpdateMgr;
    };


    typedef typename Types::CtrTypes                                            CtrTypes;
    typedef Ctr<CtrTypes>                                                       Type;

};




}}
