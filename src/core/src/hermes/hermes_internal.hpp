
// Copyright 2022 Victor Smirnov
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

#include <memoria/core/hermes/hermes.hpp>

namespace memoria::hermes {

class HermesCtrImpl final: public HermesCtr, public pool::enable_shared_from_this<HermesCtrImpl> {
    arena::ArenaAllocator arena_;

    LWMemHolder view_mem_holder_;

    friend class HermesCtrBuilder;
    friend class HermesCtr;

public:

    HermesCtrImpl()
    {
        HermesCtr::arena_ = &arena_;
        mem_holder_ = &view_mem_holder_;

        header_ = arena_.allocate_object_untagged<DocumentHeader>();
    }

    HermesCtrImpl(size_t chunk_size, arena::AllocationType alc_type = arena::AllocationType::MULTI_CHUNK):
        arena_(alc_type, chunk_size)
    {
        HermesCtr::arena_ = &arena_;
        mem_holder_ = &view_mem_holder_;

        if (alc_type == arena::AllocationType::MULTI_CHUNK) {
            header_ = arena_.allocate_object_untagged<DocumentHeader>();
        }
    }

    HermesCtrImpl(arena::AllocationType alc_type, size_t chunk_size, const void* data, size_t size):
        arena_(alc_type, chunk_size, data, size)
    {
        HermesCtr::arena_ = &arena_;
        mem_holder_ = &view_mem_holder_;

        header_ = ptr_cast<DocumentHeader>(arena_.head().memory.get());
    }

    void object_pool_init_state();
    void reset_state() noexcept;
protected:

    void configure_refholder(SharedPtrHolder* ref_holder) {
        view_mem_holder_.set_ctr(this);
        view_mem_holder_.set_owner(ref_holder);
    }
};




}
