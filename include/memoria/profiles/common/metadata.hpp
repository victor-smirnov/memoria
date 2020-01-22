
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

#include <memoria/core/exceptions/exceptions.hpp>

#include <memoria/profiles/common/common.hpp>
#include <memoria/profiles/common/block_operations.hpp>
#include <memoria/profiles/common/container_operations.hpp>

#include <memory>
#include <tuple>
#include <mutex>
#include <functional>

namespace memoria {

template <typename T, typename... Args>
auto metadata_make_shared(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

template<typename Profile> class ProfileMetadata;

template<typename Profile>
using ProfileMetadataPtr = std::shared_ptr<ProfileMetadata<Profile>>;

template<typename Profile>
class ProfileMetadataStore {

    using Key = std::pair<uint64_t, uint64_t>;

    struct hash {
        template <typename T1, typename T2>
        size_t operator()(const std::pair<T1, T2>& pp) const noexcept {
            return std::hash<T1>()(pp.first) ^ std::hash<T2>()(pp.second);
        }
    };

    using BlockMetadataMap      = std::unordered_map<Key, BlockOperationsPtr<Profile>, hash>;
    using ContainerMetadataMap  = std::unordered_map<uint64_t, ContainerOperationsPtr<Profile>>;
    using ContainerFactoryMap   = std::unordered_map<U8String, ContainerInstanceFactoryPtr<Profile>>;


    BlockMetadataMap block_map_;
    ContainerMetadataMap container_map_;
    ContainerFactoryMap factory_map_;

    mutable std::mutex mutex_;
    using LockT = std::lock_guard<std::mutex>;

public:
    template<typename>
    friend class ProfileMetadata;

    ProfileMetadataStore() {}
    ProfileMetadataStore(const ProfileMetadataStore&) = delete;
    ProfileMetadataStore(ProfileMetadataStore&&)      = delete;

    void add_block_operations(
            uint64_t ctr_type_hash,
            BlockOperationsPtr<Profile> block_meta
    )
    {
        LockT lock(mutex_);

        uint64_t block_type_hash = block_meta->block_type_hash();
        Key key{ctr_type_hash, block_type_hash};

        if (block_map_.find(key) == block_map_.end()) {
            block_map_[key] = std::move(block_meta);
        }
        else {
            MMA_THROW(RuntimeException())
                    << format_ex(
                           "Block operations already registered for {} {}", ctr_type_hash, block_type_hash
                    );
        }
    }

    void add_container_operations(
            uint64_t ctr_type_hash,
            ContainerOperationsPtr<Profile> ctr_interface
    )
    {
        LockT lock(mutex_);

        if (container_map_.find(ctr_type_hash) == container_map_.end()) {
            container_map_[ctr_type_hash] = std::move(ctr_interface);
        }
        else {
            MMA_THROW(RuntimeException())
                    << format_ex(
                           "Container interface already registered for {}",
                           ctr_type_hash
                    );
        }
    }

    void add_container_factories(
            const U8String& typedecl_signature,
            ContainerInstanceFactoryPtr<Profile> ctr_factories
    )
    {
        LockT lock(mutex_);

        if (factory_map_.find(typedecl_signature) == factory_map_.end()) {
            factory_map_[typedecl_signature] = std::move(ctr_factories);
        }
        else {
            MMA_THROW(RuntimeException())
                    << format_ex(
                           "Container instance factories already registered for {}",
                           typedecl_signature
                    );
        }
    }

    static ProfileMetadataStore<Profile>& global();

    struct Init {
        Init() {
            ProfileMetadataStore<Profile>::global();
        }
    };

private:
    void fill_to(ProfileMetadata<Profile>& meta) const;
};


template<typename Profile>
class ProfileMetadata {

    using Key = typename ProfileMetadataStore<Profile>::Key;
    using hash = typename ProfileMetadataStore<Profile>::hash;

    using BlockMetadataMap      = typename ProfileMetadataStore<Profile>::BlockMetadataMap;
    using ContainerMetadataMap  = typename ProfileMetadataStore<Profile>::ContainerMetadataMap;
    using ContainerFactoryMap   = typename ProfileMetadataStore<Profile>::ContainerFactoryMap;

    BlockMetadataMap block_map_;
    ContainerMetadataMap container_map_;
    ContainerFactoryMap factory_map_;

    template <typename>
    friend class ProfileMetadataStore;

public:
    ProfileMetadata(ProfileMetadataStore<Profile>& store) {
        store.fill_to(*this);
    }

    ProfileMetadata(const ProfileMetadata&) = delete;
    ProfileMetadata(ProfileMetadata&&)      = delete;

    const BlockOperationsPtr<Profile>& get_block_operations(uint64_t ctr_type_hash, uint64_t block_type_hash) const
    {
        Key key{ctr_type_hash, block_type_hash};
        auto ii = block_map_.find(key);
        if (ii != block_map_.end())
        {
            return ii->second;
        }
        else {
            MMA_THROW(RuntimeException())
                    << format_ex("BlockOperations is not found for {} {}", ctr_type_hash, block_type_hash);
        }
    }

    const ContainerOperationsPtr<Profile>& get_container_operations(uint64_t ctr_type_hash) const
    {
        auto ii = container_map_.find(ctr_type_hash);
        if (ii != container_map_.end())
        {
            return ii->second;
        }
        else {
            MMA_THROW(RuntimeException())
                    << format_ex("ContainerOperations is not found for {}", ctr_type_hash);
        }
    }

    const ContainerInstanceFactoryPtr<Profile>& get_container_factories(const U8String& signature) const
    {
        auto ii = factory_map_.find(signature);
        if (ii != factory_map_.end())
        {
            return ii->second;
        }
        else {
            MMA_THROW(RuntimeException())
                    << format_ex("ContainerFactories is not found for {}", signature);
        }
    }

    static const ProfileMetadataPtr<Profile>& local();

    static void init() {
        local();
    }

    void refresh() {
        ProfileMetadataStore<Profile>::global().fill_to(*this);
    }
};

template <typename Profile>
void ProfileMetadataStore<Profile>::fill_to(ProfileMetadata<Profile>& meta) const
{
    LockT lock(mutex_);

    meta.block_map_ = block_map_;
    meta.container_map_ = container_map_;
    meta.factory_map_ = factory_map_;
}

}
