
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

#include <memoria/core/types.hpp>
#include <memoria/core/tools/uuid.hpp>
#include <memoria/core/memory/memory.hpp>

#include <memoria/core/strings/string.hpp>

#include <memoria/profiles/common/common.hpp>

#include <memoria/core/iovector/io_vector.hpp>

#include <memoria/core/tools/optional.hpp>

#include <memoria/core/tools/result.hpp>


#include <string>
#include <functional>

namespace memoria {

template <typename Profile>
struct CtrBlock;

template <typename Profile>
using CtrBlockPtr = std::shared_ptr<CtrBlock<Profile>>;


template <typename Profile>
struct CtrBlock {
    using ApiBlockID = ApiProfileBlockID<Profile>;

    virtual ~CtrBlock() noexcept = default;

    virtual std::vector<CtrBlockPtr<Profile>> children() const = 0;
    virtual void describe(std::ostream& out) const = 0;

    virtual bool is_leaf() const = 0;
    virtual ApiBlockID block_id() const = 0;
};


template <typename Profile>
struct CtrReferenceableBase {

    using CtrID = ApiProfileCtrID<Profile>;

    virtual ~CtrReferenceableBase() noexcept = default;

    virtual bool is_castable_to(uint64_t type_hash) const noexcept  = 0;
    virtual U8String describe_type() const                          = 0;
    virtual U8String describe_datatype() const                      = 0;
    virtual uint64_t type_hash() noexcept                           = 0;

    virtual const std::type_info& api_type_info() const noexcept    = 0;

    virtual void set_new_block_size(int32_t block_size)             = 0;
    virtual int32_t get_new_block_size() const                      = 0;

    virtual Optional<U8String> get_ctr_property(U8StringView key) const = 0;
    virtual void set_ctr_property(U8StringView key, U8StringView value) = 0;
    virtual void remove_ctr_property(U8StringView key) = 0;

    virtual size_t ctr_properties() const = 0;
    virtual void for_each_ctr_property(std::function<void (U8StringView, U8StringView)> consumer) const = 0;
    virtual void set_ctr_properties(const std::vector<std::pair<U8String, U8String>>& entries) = 0;

    virtual Optional<CtrID> get_ctr_reference(U8StringView key) const = 0;
    virtual void set_ctr_reference(U8StringView key, const CtrID& value) = 0;
    virtual void remove_ctr_reference(U8StringView key) = 0;
    virtual size_t ctr_references() const = 0;
    virtual void for_each_ctr_reference(std::function<void (U8StringView, const CtrID&)> consumer) const = 0;
    virtual void set_ctr_references(const std::vector<std::pair<U8String, CtrID>>& entries) = 0;
    
    virtual const CtrID& name() const noexcept = 0;

    virtual std::shared_ptr<io::IOVector> create_iovector() = 0;

    virtual void drop() = 0;
    virtual void flush() = 0;

    virtual CtrBlockPtr<Profile> root_block() = 0;

    virtual void internal_unref_cascade(const ApiProfileBlockID<Profile>& block_id) = 0;

    virtual void internal_reset_allocator_holder() noexcept = 0;
};

template <typename Profile>
struct CtrReferenceable: CtrReferenceableBase<Profile> {
    virtual CtrSharedPtr<CtrReferenceable<Profile>> shared_self() noexcept = 0;
};

}
