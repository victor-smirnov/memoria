
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

#include <memoria/v1/core/types.hpp>
#include <memoria/v1/core/tools/uuid.hpp>
#include <memoria/v1/core/tools/memory.hpp>

#include <memoria/v1/core/strings/string.hpp>

#include <memoria/v1/profiles/common/common.hpp>

#include <memoria/v1/core/iovector/io_vector.hpp>

#include <memoria/v1/core/tools/optional.hpp>

#include <memoria/v1/core/tools/result.hpp>


#include <string>

namespace memoria {
namespace v1 {

template <typename Profile>
struct CtrReferenceable {

    using CtrID = ProfileCtrID<Profile>;

    virtual ~CtrReferenceable() noexcept {}

    virtual bool is_castable_to(uint64_t type_hash) const noexcept  = 0;
    virtual U8String describe_type() const noexcept                 = 0;
    virtual U8String describe_datatype() const noexcept             = 0;
    virtual uint64_t type_hash() noexcept                           = 0;

    virtual VoidResult set_new_block_size(int32_t block_size) noexcept       = 0;
    virtual Result<int32_t> get_new_block_size() const noexcept              = 0;

    virtual Result<Optional<U8String>> get_ctr_property(U8StringView key) const noexcept  = 0;
    virtual VoidResult set_ctr_property(U8StringView key, U8StringView value) noexcept = 0;
    virtual VoidResult remove_ctr_property(U8StringView key) noexcept = 0;

    virtual Result<size_t> ctr_properties() const noexcept = 0;
    virtual VoidResult for_each_ctr_property(std::function<void (U8StringView, U8StringView)> consumer) const noexcept = 0;
    virtual VoidResult set_ctr_properties(const std::vector<std::pair<U8String, U8String>>& entries) noexcept = 0;

    virtual Result<Optional<CtrID>> get_ctr_reference(U8StringView key) const noexcept = 0;
    virtual VoidResult set_ctr_reference(U8StringView key, const CtrID& value) noexcept = 0;
    virtual VoidResult remove_ctr_reference(U8StringView key) noexcept = 0;
    virtual Result<size_t> ctr_references() const noexcept = 0;
    virtual VoidResult for_each_ctr_reference(std::function<VoidResult (U8StringView, const CtrID&)> consumer) const noexcept = 0;
    virtual VoidResult set_ctr_references(const std::vector<std::pair<U8String, CtrID>>& entries) noexcept = 0;
    
    virtual const ProfileCtrID<Profile>& name() const noexcept = 0;

    virtual std::shared_ptr<io::IOVector> create_iovector() noexcept  = 0;

    virtual VoidResult drop() noexcept  = 0;
    virtual VoidResult flush() noexcept = 0;

    virtual CtrSharedPtr<CtrReferenceable<Profile>> shared_self() noexcept = 0;
};

}}
