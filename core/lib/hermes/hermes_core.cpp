
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


#include <memoria/core/hermes/container.hpp>
#include <memoria/core/hermes/object.hpp>

#include <memoria/core/arena/hash_fn.hpp>
#include <memoria/core/arena/string.hpp>

#include <memoria/core/hermes/path/path.h>

#include <memoria/core/reflection/type_signature.hpp>

#include "hermes_internal.hpp"

namespace memoria::hermes {

void HermesCtrImpl::object_pool_init_state() {
    if (!header_) {
        arena_.object_pool_init_state();
        header_ = arena_.allocate_object_untagged<DocumentHeader>();
    }
}

void HermesCtrImpl::reset_state() noexcept {
    arena_.reset_state();
    header_ = nullptr;
}

HermesCtr HermesCtrView::make_pooled(ObjectPools& pool)
{
    auto arena = get_reusable_shared_instance<arena::PoolableArena>(pool);
    auto ctr = HermesCtr{&arena->mem_holder(), arena.get()};
    ctr.header_ = arena->allocate_object_untagged<DocumentHeader>();
    return ctr;
}

HermesCtr HermesCtrView::make_new(size_t initial_capacity)
{
    auto arena = TL_allocate_shared<arena::PoolableArena>(initial_capacity);
    auto ctr = HermesCtr{&arena->mem_holder(), arena.get()};
    ctr.header_ = arena->allocate_object_untagged<DocumentHeader>();
    return ctr;
}

HermesCtr HermesCtrView::from_span(Span<const uint8_t> data) {
    auto arena = TL_allocate_shared<arena::PoolableArena>(
        arena::AllocationType::MULTI_CHUNK,
        4096,
        data.data(),
        data.size()
    );

    auto ctr = HermesCtr{&arena->mem_holder()};
    return ctr;
}

HermesCtr HermesCtrView::from_buffer(UniquePtr<uint8_t>&& data, size_t size, size_t header_size) {
    auto arena = TL_allocate_shared<arena::PoolableArena>(
        arena::AllocationType::MULTI_CHUNK,
        4096,
        std::move(data),
        size,
        header_size
    );

    auto ctr = HermesCtr{&arena->mem_holder()};
    return ctr;
}


RawStringEscaper& RawStringEscaper::current() {
    static thread_local RawStringEscaper escaper;
    return escaper;
}

StringEscaper& StringEscaper::current() {
    static thread_local StringEscaper escaper;
    return escaper;
}


std::ostream& operator<<(std::ostream& out, Object ptr) {
    out << ptr.to_string();
    return out;
}

std::ostream& operator<<(std::ostream& out, Datatype ptr) {
    out << ptr.to_string();
    return out;
}

uint64_t ParameterView::hash_code() const {
    assert_not_null();
    arena::DefaultHashFn<U8StringView> hash;
    return hash(dt_ctr_->view());
}

Object ObjectView::search(U8StringView query) const
{
    auto ast = HermesCtrView::parse_hermes_path(query);
    auto exp = ast.root().value().as_tiny_object_map();
    return hermes::path::search(exp, Object(get_mem_holder(), storage_.addr));
}

Object ObjectView::search(U8StringView query, const IParameterResolver& params) const
{
    auto ast = HermesCtrView::parse_hermes_path(query);
    auto exp = ast.root().value().as_tiny_object_map();
    return hermes::path::search(exp, Object(get_mem_holder(), storage_.addr), params);
}

namespace {

constexpr size_t HEADER_SIZE = 16;

template <size_t Size>
using PoolT = pool::SimpleObjectPool<SizedHermesCtrImpl<HEADER_SIZE + Size>>;


}

PoolSharedPtr<StaticHermesCtrImpl> HermesCtrView::get_ctr_instance(size_t size)
{

    static thread_local LocalSharedPtr<PoolT<8>> instance_pool_8     = MakeLocalShared<PoolT<8>>();
    static thread_local LocalSharedPtr<PoolT<32>> instance_pool_32   = MakeLocalShared<PoolT<32>>();
    static thread_local LocalSharedPtr<PoolT<64>> instance_pool_64   = MakeLocalShared<PoolT<64>>();
    static thread_local LocalSharedPtr<PoolT<128>> instance_pool_128 = MakeLocalShared<PoolT<128>>();
    static thread_local LocalSharedPtr<PoolT<256>> instance_pool_256 = MakeLocalShared<PoolT<256>>();

    if (size <= 256) {
        if (size <= HEADER_SIZE + 8) {
            return instance_pool_8->allocate_shared();
        }
        else if (size <= HEADER_SIZE + 32) {
            return instance_pool_32->allocate_shared();
        }
        else if (size <= HEADER_SIZE + 64) {
            return instance_pool_64->allocate_shared();
        }
        else if (size <= HEADER_SIZE + 128) {
            return instance_pool_128->allocate_shared();
        }
        else if (size <= HEADER_SIZE + 256) {
            return instance_pool_256->allocate_shared();
        }
    }

    return PoolSharedPtr<StaticHermesCtrImpl>{};
}

void HermesCtrView::init_from(arena::ArenaAllocator& arena) {
    if (arena_) {
        arena_->init_from(arena);
        header_ = ptr_cast<DocumentHeader>(arena.tail().memory.get());
    }
    else {
        MEMORIA_MAKE_GENERIC_ERROR("HermesCtrView instance is immutable").do_throw();
    }
}


template <size_t Size>
using ViewPoolT = AlignedSpacePool<TaggedGenericView::VIEW_ALIGN_OF, Size, TaggedGenericView>;

PoolSharedPtr<TaggedGenericView> TaggedGenericView::allocate_space(size_t size)
{
    constexpr size_t MAX = TaggedGenericView::MAX_VIEW_SIZE;
    constexpr size_t SMALL  = 16;
    constexpr size_t MEDIUM = 32;

    static_assert(SMALL <= MAX && MEDIUM <= MAX);

    static thread_local LocalSharedPtr<ViewPoolT<SMALL>>  view_pool_small  = MakeLocalShared<ViewPoolT<SMALL>>();
    static thread_local LocalSharedPtr<ViewPoolT<MEDIUM>> view_pool_medium = MakeLocalShared<ViewPoolT<MEDIUM>>();
    static thread_local LocalSharedPtr<ViewPoolT<MAX>>    view_pool_max    = MakeLocalShared<ViewPoolT<MAX>>();

    if (size <= SMALL) {
        return view_pool_small->allocate_shared();
    }
    else if (size <= MEDIUM) {
        return view_pool_medium->allocate_shared();
    }
    else if (size <= MAX) {
        return view_pool_max->allocate_shared();
    }
    else {
        MEMORIA_MAKE_GENERIC_ERROR("Requested view size of {} is too large, max is {}", size, MAX).do_throw();
    }
}


HermesCtr TypedValueView::ctr() {
    assert_not_null();
    return HermesCtr(mem_holder_);
}


void CheckStructureState::check_ptr(const arena::EmbeddingRelativePtr<void>& ptr, const char* src)
{
    if (ptr.is_pointer())
    {
        if (ptr.is_not_null())
        {
            check_bounds(ptr.get(), src);
            if (!cycle_bitmap_.test_bit(offset(ptr.get())))
            {
                auto tag = arena::read_type_tag(ptr.get());
                get_type_reflection(tag).hermes_check(ptr.get(), *this, src);
            }
        }
    }
    else {
        auto tag = ptr.get_tag();
        get_type_reflection(ShortTypeCode::of_object(tag)).hermes_check_embedded(ptr, *this, src);
    }
}

void CheckStructureState::check_ptr(const arena::RelativePtr<void>& ptr, const char* src)
{
    check_ptr(ptr.get(), src);
}

void CheckStructureState::check_ptr(const void* ptr, const char* src)
{
    if (ptr)
    {
        check_bounds(ptr, src);
        if (!cycle_bitmap_.test_bit(offset(ptr)))
        {
            auto tag = arena::read_type_tag(ptr);
            get_type_reflection(tag).hermes_check(ptr, *this, src);
        }
    }
}

void CheckStructureState::check_and_set_tagged(const void* ptr, size_t size, const char* src)
{
    auto tag = arena::read_type_tag(ptr);
    size_t start = offset(ptr) - tag.full_code_len();
    if (!allocation_bitmap_.check_and_set(start, start + size + tag.full_code_len())) {
        MEMORIA_MAKE_GENERIC_ERROR(
                    "Allocation from {} to {} overlaps with other allocations at {}",
                    start, start + size, src
        ).do_throw();
    }
}

void CheckStructureState::check_and_set(const void* ptr, size_t size, const char* src)
{
    size_t start = offset(ptr);
    if (!allocation_bitmap_.check_and_set(start, start + size)) {
        MEMORIA_MAKE_GENERIC_ERROR(
                    "Allocation from {} to {} overlaps with other allocations at {}",
                    start, start + size, src
        ).do_throw();
    }
}


}

namespace memoria {

TypeSignature::TypeSignature(U8StringView name) {
    name_ = hermes::HermesCtrView::parse_datatype(name).root().value().as_datatype().to_string();
}

TypeSignature TypeSignature::make_normalized(U8StringView str) {
    return TypeSignature(normalize_type_declaration(str), 0);
}

hermes::HermesCtr TypeSignature::parse() const {
    return hermes::HermesCtrView::parse_datatype(name_);
}

hermes::HermesCtr TypeSignature::parse(U8StringView str) {
    return hermes::HermesCtrView::parse_datatype(str);
}

}
