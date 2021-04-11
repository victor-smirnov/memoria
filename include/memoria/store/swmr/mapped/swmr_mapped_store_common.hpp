
// Copyright 2020-2021 Victor Smirnov
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

#include <memoria/store/swmr/common/swmr_store_commit_base.hpp>

namespace memoria {

/*
template <typename Profile> class MappedSWMRStore;

template <typename Profile>
class MappedSWMRStoreCommitBase: public SWMRStoreCommitBase<Profile>
{
    using Base = SWMRStoreCommitBase<Profile>;
protected:

    using typename Base::BlockType;
    using typename Base::BlockG;
    using typename Base::BlockID;
    using typename Base::ApiProfileT;
    using typename Base::CommitDescriptorT;
    using typename Base::Superblock;

    using Store = MappedSWMRStore<Profile>;

    static constexpr int32_t BASIC_BLOCK_SIZE = Store::BASIC_BLOCK_SIZE;

    SharedPtr<Store> store_;
    Span<uint8_t> buffer_;

    template <typename> friend class SWMRMappedStoreHistoryView;

public:
    MappedSWMRStoreCommitBase(
            MaybeError& maybe_error,
            SharedPtr<Store> store,
            Span<uint8_t> buffer,
            CommitDescriptorT* commit_descriptor,
            ReferenceCounterDelegate<Profile>* refcounter_delegate
    ) noexcept:
        Base(commit_descriptor, refcounter_delegate),
        store_(store),
        buffer_(buffer)
    {}


    virtual Result<BlockG> getBlock(const BlockID& id) noexcept
    {
        using ResultT = Result<BlockG>;
        BlockType* block = ptr_cast<BlockType>(buffer_.data() + id.value() * BASIC_BLOCK_SIZE);
        return ResultT::of(BlockG{block});
    }
};

*/

}
