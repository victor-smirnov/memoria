
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

#include <memoria/core/types.hpp>
#include <memoria/core/tools/uid_256.hpp>
#include <memoria/core/strings/string.hpp>

namespace memoria {

struct StoreEventListener {
    virtual ~StoreEventListener() noexcept {}

    virtual void shapshot_created(UID256 snapshot_id) = 0;
    virtual void shapshot_removed(UID256 snapshot_id) = 0;

    virtual void branch_created(const U8String& branch_name) = 0;
    virtual void branch_removed(const U8String& branch_name) = 0;
    virtual void branch_updated(const U8String& branch_name) = 0;


    virtual void container_created(UID256 snapshot_id, UID256 container_name) = 0;
    virtual void container_removed(UID256 snapshot_id, UID256 container_name) = 0;
    virtual void container_updated(UID256 snapshot_id, UID256 container_name) = 0;

    virtual void block_created(UID256 snapshot_id, UID256 container_name, UID256 block_id) = 0;
    virtual void block_removed(UID256 snapshot_id, UID256 container_name, UID256 block_id) = 0;
};

}
