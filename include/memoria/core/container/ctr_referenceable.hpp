
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
#include <memoria/core/tools/any_id.hpp>

#include <memoria/core/tools/checks.hpp>

#include <string>
#include <functional>

#define MEMORIA_READ_ONLY_API { MEMORIA_MAKE_GENERIC_ERROR("Read-only container").do_throw(); }

namespace memoria {



template <typename Profile>
struct CtrBlock;

template <typename Profile>
using CtrBlockPtr = std::shared_ptr<CtrBlock<Profile>>;


template <typename Profile>
struct CtrBlock {

    virtual ~CtrBlock() noexcept = default;

    virtual std::vector<CtrBlockPtr<Profile>> children() const = 0;
    virtual void describe(std::ostream& out) const = 0;

    virtual bool is_leaf() const = 0;
    virtual AnyID block_id() const = 0;
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

    virtual void set_new_block_size(int32_t block_size) MEMORIA_READ_ONLY_API
    virtual int32_t get_new_block_size() const                      = 0;

    virtual Optional<U8String> get_ctr_property(U8StringView key) const = 0;
    virtual void set_ctr_property(U8StringView key, U8StringView value) MEMORIA_READ_ONLY_API
    virtual void remove_ctr_property(U8StringView key) MEMORIA_READ_ONLY_API

    virtual size_t ctr_properties() const = 0;
    virtual void for_each_ctr_property(std::function<void (U8StringView, U8StringView)> consumer) const = 0;
    virtual void set_ctr_properties(const std::vector<std::pair<U8String, U8String>>& entries) MEMORIA_READ_ONLY_API

    virtual Optional<CtrID> get_ctr_reference(U8StringView key) const = 0;
    virtual void set_ctr_reference(U8StringView key, const CtrID& value) MEMORIA_READ_ONLY_API
    virtual void remove_ctr_reference(U8StringView key) MEMORIA_READ_ONLY_API
    virtual size_t ctr_references() const = 0;
    virtual void for_each_ctr_reference(std::function<void (U8StringView, const CtrID&)> consumer) const = 0;
    virtual void set_ctr_references(const std::vector<std::pair<U8String, CtrID>>& entries) MEMORIA_READ_ONLY_API
    
    virtual const CtrID& name() const noexcept = 0;

    virtual IterSharedPtr<io::IOVector> create_iovector() = 0;

    virtual void drop()    MEMORIA_READ_ONLY_API
    virtual void cleanup() MEMORIA_READ_ONLY_API
    virtual void flush()   MEMORIA_READ_ONLY_API

    virtual void check(const CheckResultConsumerFn& fn) = 0;

    virtual CtrBlockPtr<Profile> root_block() = 0;

    virtual void dump_leafs(ApiProfileCtrSizeT<Profile> leafs = -1) = 0;

    // Don't use in an application code
    virtual void internal_unref_cascade(const AnyID& block_id) MEMORIA_READ_ONLY_API
    virtual void internal_detouch_from_store() noexcept = 0;
    virtual void internal_attach_to_store(SnpSharedPtr<IStoreApiBase<Profile>>) noexcept = 0;
    virtual void internal_configure_shared_from_this(pool::detail::ObjectPoolRefHolder*) noexcept = 0;
};

template <typename Profile>
struct CtrReferenceable: CtrReferenceableBase<Profile> {
    virtual CtrSharedPtr<CtrReferenceable<Profile>> shared_self() noexcept = 0;
};

}
