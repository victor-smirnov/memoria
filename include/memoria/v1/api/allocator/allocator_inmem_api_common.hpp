
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

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/tools/strings/strings.hpp>
#include <memoria/v1/core/tools/stream.hpp>
#include <memoria/v1/core/tools/uuid.hpp>
#include <memoria/v1/core/tools/memory.hpp>

#include <memoria/v1/api/common/ctr_api.hpp>
#include <memoria/v1/core/container/container.hpp>
#include <memoria/v1/core/container/allocator.hpp>
#include <memoria/v1/core/container/logs.hpp>

#include <memoria/v1/allocators/inmem/common/container_collection_cfg.hpp>

#include <memoria/v1/filesystem/path.hpp>



namespace memoria {
namespace v1 {
    

enum class SnapshotStatus {ACTIVE, COMMITTED, DROPPED, DATA_LOCKED};

template <typename TxnId>
class SnapshotMetadata {
	TxnId parent_id_;
	TxnId snapshot_id_;
	std::vector<TxnId> children_;
    U16String description_;
	SnapshotStatus status_;
public:
    SnapshotMetadata(const TxnId& parent_id, const TxnId& snapshot_id, const std::vector<TxnId>& children, U16StringRef description, SnapshotStatus status):
		parent_id_(parent_id),
		snapshot_id_(snapshot_id),
		children_(children),
		description_(description),
		status_(status)
	{}

    const TxnId& parent_id() const              {return parent_id_;}
    const TxnId& snapshot_id() const            {return snapshot_id_;}
    const std::vector<TxnId>& children() const 	{return children_;}
    const U16String& description() const        {return description_;}
    SnapshotStatus status() const               {return status_;}
};



}
}
