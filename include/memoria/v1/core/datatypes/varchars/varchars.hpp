
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

#include <memoria/v1/core/types.hpp>


#include <memoria/v1/core/datatypes/datum.hpp>

#include <memoria/v1/api/common/packed_api.hpp>

#include <memoria/v1/core/tools/span.hpp>
#include <memoria/v1/core/tools/arena_buffer.hpp>
#include <memoria/v1/core/strings/u8_string.hpp>

#include <memoria/v1/core/datatypes/varchars/varchar_dt.hpp>

#include <tuple>

namespace memoria {
namespace v1 {




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



}}
