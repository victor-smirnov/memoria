
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

#include <memoria/core/types.hpp>

#include <memoria/prototypes/bt_fl/btfl_names.hpp>
#include <memoria/prototypes/bt_fl/btfl_tools.hpp>

#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>



#include <iostream>

namespace memoria {


MEMORIA_V1_ITERATOR_PART_BEGIN(btfl::IteratorMiscName)

    using typename Base::TreePathT;
    using typename Base::CtrSizeT;
    using typename Base::Container;
    using DataSizesT = typename Container::Types::DataSizesT;

    static const int32_t Streams                = Container::Types::Streams;
    static const int32_t DataStreams            = Container::Types::DataStreams;
    static const int32_t StructureStreamIdx     = Container::Types::StructureStreamIdx;

public:

    virtual const std::type_info& cxx_type() const {
        return typeid(MyType);
    }

    virtual int32_t iovector_pos() const {
        return self().iter_local_pos();
    }


    void iter_refresh()
    {
        return self().iter_refresh_branch_prefixes();
    }

    void iter_init()
    {
        Base::iter_init();

        self().iter_stream() = StructureStreamIdx;
    }

MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(btfl::IteratorMiscName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS


#undef M_TYPE
#undef M_PARAMS

}
