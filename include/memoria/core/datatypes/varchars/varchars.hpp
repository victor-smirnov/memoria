
// Copyright 2019 Victor Smirnov
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


#include <memoria/core/datatypes/datum.hpp>

#include <memoria/api/common/packed_api.hpp>

#include <memoria/core/tools/span.hpp>
#include <memoria/core/tools/arena_buffer.hpp>
#include <memoria/core/strings/u8_string.hpp>

#include <memoria/core/datatypes/varchars/varchar_dt.hpp>

#include <tuple>

namespace memoria {

class VarcharStorage: public DatumStorageBase<Varchar, typename DataTypeTraits<Varchar>::DatumStorageSelector> {
    using SelectorTag = typename DataTypeTraits<Varchar>::DatumStorageSelector;

    using Base = DatumStorageBase<Varchar, SelectorTag>;
    using typename Base::ViewType;
public:
    VarcharStorage(ViewType view) noexcept: Base(view) {}

    virtual void destroy() noexcept;
    static VarcharStorage* create(ViewType view);
    virtual U8String to_sdn_string() const;
};

}
