
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
#include <memoria/core/hermes/value.hpp>

#include <memoria/core/arena/hash_fn.hpp>
#include <memoria/core/arena/string.hpp>
#include <memoria/core/hermes/data_object.hpp>

#include <memoria/core/hermes/path/path.h>

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

pool::SharedPtr<HermesCtr> HermesCtr::make_pooled(ObjectPools& pool) {
    return get_reusable_shared_instance<HermesCtrImpl>(pool);
}

pool::SharedPtr<HermesCtr> HermesCtr::make_new(size_t initial_capacity) {
    return TL_allocate_shared<HermesCtrImpl>(initial_capacity);
}


RawStringEscaper& RawStringEscaper::current() {
    static thread_local RawStringEscaper escaper;
    return escaper;
}

StringEscaper& StringEscaper::current() {
    static thread_local StringEscaper escaper;
    return escaper;
}


std::ostream& operator<<(std::ostream& out, ValuePtr ptr) {
    out << ptr->to_string();
    return out;
}

std::ostream& operator<<(std::ostream& out, DatatypePtr ptr) {
    out << ptr->to_string();
    return out;
}

uint64_t Parameter::hash_code() const {
    assert_not_null();
    arena::DefaultHashFn<U8StringView> hash;
    return hash(dt_ctr_->view());
}

ValuePtr Value::search(U8StringView query) const
{
    hermes::path::Expression exp(std::string{query});
    return hermes::path::search(exp, ValuePtr(Value(value_storage_.addr, doc_, get_ptr_holder())));
}

ValuePtr Value::search(U8StringView query, const IParameterResolver& params) const
{
    hermes::path::Expression exp(std::string{query});
    return hermes::path::search(exp, ValuePtr(Value(value_storage_.addr, doc_, get_ptr_holder())), params);
}

namespace {

constexpr size_t HEADER_SIZE = 16;

template <size_t Size>
using PoolT = pool::SimpleObjectPool<SizedHermesCtrImpl<HEADER_SIZE + Size>>;


}

PoolSharedPtr<StaticHermesCtrImpl> HermesCtr::get_ctr_instance(size_t size)
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

void HermesCtr::init_from(arena::ArenaAllocator& arena) {
    if (arena_) {
        arena_->init_from(arena);
        header_ = ptr_cast<DocumentHeader>(arena.tail().memory.get());
    }
    else {
        MEMORIA_MAKE_GENERIC_ERROR("HermesCtr instance is immutable").do_throw();
    }
}


}
