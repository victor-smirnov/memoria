
// Copyright 2015 Victor Smirnov
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

#include <memoria/v1/core/types/types.hpp>

#include <memoria/v1/prototypes/bt_fl/btfl_names.hpp>
#include <memoria/v1/prototypes/bt_fl/btfl_tools.hpp>

#include <memoria/v1/prototypes/bt_fl/io/btfl_output.hpp>



#include <memoria/v1/core/container/iterator.hpp>
#include <memoria/v1/core/container/macros.hpp>



#include <iostream>

namespace memoria {
namespace v1 {


MEMORIA_V1_ITERATOR_PART_BEGIN(v1::btfl::IteratorMiscName)

    using Container = typename Base::Container;

    using CtrSizeT      = typename Container::Types::CtrSizeT;
    using DataSizesT    = typename Container::Types::DataSizesT;

    static const Int Streams                = Container::Types::Streams;
    static const Int DataStreams            = Container::Types::DataStreams;
    static const Int StructureStreamIdx     = Container::Types::StructureStreamIdx;



protected:



//    void refresh()
//    {
//        Base::refresh();
//    }
//
//    void refresh_prefixes()
//    {
//        Base::refresh();
//    }

    void refresh()
    {
        self().refreshBranchPrefixes();
    }


    void checkPrefix() {
        auto tmp = self();

        tmp.refresh();

        MEMORIA_V1_ASSERT(self().cache(), ==, tmp.cache());
    }

//    void prepare() {
//        Base::prepare();
//
//        auto& self = this->self();
//        auto& cache = self.cache();
//    }


    void init()
    {
        Base::init();
    }

MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(v1::btfl::IteratorMiscName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS


#undef M_TYPE
#undef M_PARAMS

}}
