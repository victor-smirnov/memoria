
// Copyright 2017 Victor Smirnov
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

#include <memoria/api/common/ctr_api_btss.hpp>
#include <memoria/api/common/iobuffer_adatpters.hpp>

#include <memoria/core/datatypes/traits.hpp>
#include <memoria/core/datatypes/encoding_traits.hpp>
#include <memoria/core/datatypes/io_vector_traits.hpp>

#include <memoria/core/types/typehash.hpp>

#include <memoria/core/iovector/io_vector.hpp>

#include <memoria/api/allocation_map/allocation_map_scanner.hpp>
#include <memoria/api/allocation_map/allocation_map_producer.hpp>
#include <memoria/api/allocation_map/allocation_map_api_factory.hpp>

#include <memoria/core/strings/string_codec.hpp>

#include <memoria/core/datatypes/buffer/buffer.hpp>

namespace memoria {

template <typename Profile>
struct AllocationMapIterator: BTSSIterator<Profile> {

    virtual ~AllocationMapIterator() noexcept {}

    virtual bool is_end() const noexcept = 0;
    virtual BoolResult next() noexcept = 0;
};




template <typename Profile>
struct ICtrApi<AllocationMap, Profile>: public CtrReferenceable<Profile> {
    using ApiTypes  = ICtrApiTypes<AllocationMap, Profile>;

    virtual Result<ProfileCtrSizeT<Profile>> size() const noexcept = 0;


    MMA_DECLARE_ICTRAPI();
};

}
