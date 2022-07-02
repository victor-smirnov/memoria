
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

#include <memoria/prototypes/bt_ss/btss_factory.hpp>

#include <memoria/containers/vector/vctr_tools.hpp>
#include <memoria/containers/vector/vctr_names.hpp>
#include <memoria/containers/vector/vector_api_impl.hpp>

#include <memoria/containers/vector/container/vctr_c_tools.hpp>
#include <memoria/containers/vector/container/vctr_c_insert.hpp>
#include <memoria/containers/vector/container/vctr_c_remove.hpp>
#include <memoria/containers/vector/container/vctr_c_api_common.hpp>
#include <memoria/containers/vector/container/vctr_c_find.hpp>

#include <memoria/containers/vector/vctr_names.hpp>

#include <memoria/core/packed/packed.hpp>

#include <memoria/api/vector/vector_api.hpp>

#include <memoria/core/datatypes/traits.hpp>
#include <memoria/core/datatypes/varchars/varchar_dt.hpp>

namespace memoria {

//template <typename Profile, typename DataType>
//struct VectorBTTypesBase: public BTTypes<Profile, BTSingleStream> {

//    using Base = BTTypes<Profile, BTSingleStream>;

//    using Value = DataType;
//    using ValueDataType = DataType;
//    using ValueView = typename DataTypeTraits<DataType>::ViewType;

//    using Entry = DataType;



//    using CommonContainerPartsList = MergeLists<
//            typename Base::CommonContainerPartsList,

//            mvector::CtrToolsName,
//            mvector::CtrInsertName,
//            mvector::CtrRemoveName,
//            mvector::CtrFindName,
//            mvector::CtrApiCommonName
//    >;
//};





template <typename Profile, typename Value_>
struct BTTypes<Profile, Vector<Value_> >:
        public BTTypes<Profile, BTSingleStream>,
        ICtrApiTypes<Vector<Value_>, Profile>
{

    using Base = BTTypes<Profile, BTSingleStream>;

    using Value = Value_;
    using ValueDataType = Value;
    using ValueView = typename DataTypeTraits<Value>::ViewType;

    using CommonContainerPartsList = MergeLists<
            typename Base::CommonContainerPartsList,

            mvector::CtrToolsName,
            mvector::CtrInsertName,
            mvector::CtrRemoveName,
            mvector::CtrFindName,
            mvector::CtrApiCommonName
    >;




    using LeafValueStruct = typename mvector::VectorValueStructTF<Value_>::Type;


    using StreamDescriptors = TL<bt::StreamTF<
        TL<
            TL<StreamSize>,
            TL<LeafValueStruct>
        >,
        bt::DefaultBranchStructTF//,
        //TL<TL<>, TL<>>
    >>;
};


template <typename Profile, typename Value, typename T>
class CtrTF<Profile, Vector<Value>, T>: public CtrTF<Profile, BTSingleStream, T> {
};

}
