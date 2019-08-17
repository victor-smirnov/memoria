
// Copyright 2018 Victor Smirnov
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
#include <memoria/v1/core/strings/string.hpp>

namespace memoria {
namespace v1 {

struct StoreEventListener {
    virtual ~StoreEventListener() noexcept {}

    virtual void shapshot_created(UUID snapshot_id) = 0;
    virtual void shapshot_removed(UUID snapshot_id) = 0;

    virtual void branch_created(const U16String& branch_name) = 0;
    virtual void branch_removed(const U16String& branch_name) = 0;
    virtual void branch_updated(const U16String& branch_name) = 0;


    virtual void container_created(UUID snapshot_id, UUID container_name) = 0;
    virtual void container_removed(UUID snapshot_id, UUID container_name) = 0;
    virtual void container_updated(UUID snapshot_id, UUID container_name) = 0;

    virtual void block_created(UUID snapshot_id, UUID container_name, UUID block_id) = 0;
    virtual void block_removed(UUID snapshot_id, UUID container_name, UUID block_id) = 0;
};

}
}
