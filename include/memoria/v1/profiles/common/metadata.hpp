
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

#include <memoria/v1/core/exceptions/exceptions.hpp>

#include <memoria/v1/profiles/common/common.hpp>
#include <memoria/v1/metadata/block.hpp>

#include <memory>
#include <tuple>



namespace memoria {
namespace v1 {

template<typename Profile> class ProfileMetadata;

template<typename Profile>
using ProfileMetadataPtr = std::shared_ptr<ProfileMetadata<Profile>>;

template<typename Profile>
class ProfileMetadata {

    using Key = std::pair<uint64_t, uint64_t>;

    struct hash {
        template <typename T1, typename T2>
        size_t operator()(const std::pair<T1, T2>& pp) const noexcept {
            return std::hash<T1>()(pp.first) ^ std::hash<T2>()(pp.second);
        }
    };

    using BlockMetadataMap = std::unordered_map<Key, BlockOperationsPtr<Profile>, hash>;

    BlockMetadataMap block_map_;

public:
    ProfileMetadata() {}
    ProfileMetadata(const ProfileMetadata&) = delete;
    ProfileMetadata(ProfileMetadata&&)      = delete;

    void add_block_metadata(
            uint64_t ctr_type_hash,
            BlockOperationsPtr<Profile> block_meta
    )
    {
        uint64_t block_type_hash = block_meta->block_type_hash();
        Key key{ctr_type_hash, block_type_hash};

        if (block_map_.find(key) == block_map_.end()) {
            block_map_[key] = std::move(block_meta);
        }
        else {
            MMA1_THROW(RuntimeException())
                    << fmt::format_ex(
                           u"Block operations already registered for {} {}", ctr_type_hash, block_type_hash
                    );
        }
    }

    static const ProfileMetadataPtr<Profile>& get_thread_local()
    {
        static thread_local ProfileMetadataPtr<Profile> metadata(std::make_shared<ProfileMetadata>());
        return metadata;
    }

    struct Init {
        Init() {
            ProfileMetadata<Profile>::get_thread_local();
        }
    };
};

}}
