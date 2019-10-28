
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


#include <memoria/v1/core/tools/arena_buffer.hpp>
#include <memoria/v1/core/types/type2type.hpp>

#include <memoria/v1/core/tools/bitmap.hpp>

#include <boost/interprocess/offset_ptr.hpp>
#include <boost/interprocess/containers/map.hpp>

#include <memoria/v1/core/exceptions/exceptions.hpp>

#include <utility>
#include <type_traits>
#include <unordered_map>

namespace memoria {
namespace v1 {

struct ArenaException: virtual RuntimeException {};
struct UnmodifiableArenaException: virtual ArenaException {};

namespace mapped_ {
    template <typename, typename = VoidT<>>
    struct HasObjectSizeHelper: std::false_type {};

    template <typename T>
    struct HasObjectSizeHelper<T, VoidT<decltype(T::UseObjectSize)>>: std::true_type {};
}


template <typename T>
constexpr bool MappedHasObjectSize = mapped_::HasObjectSizeHelper<T>::type::value;




template <typename T, typename HolderT, typename Arena>
class MappedPtr {
    HolderT ptr_value_;
public:
    using Ptr = T*;
    using ValueT = T;

    MappedPtr() = default;
    MappedPtr(HolderT value): ptr_value_(value) {}

    HolderT get() const {
        return ptr_value_;
    }

    T* get(Arena* arena) {
        return T2T<T*>(arena->data() + ptr_value_);
    }

    const T* get(const Arena* arena) const {
        return T2T<const T*>(arena->data() + ptr_value_);
    }

//    operator bool() const {
//        return ptr_value_ != 0;
//    }

    operator HolderT() const {
        return ptr_value_;
    }
};

template <typename T, typename Arena>
class MappedPtrResolver {
    T ptr_;
    Arena* arena_;
public:
    MappedPtrResolver(T ptr, Arena* arena): ptr_(ptr), arena_(arena) {}

    auto get() const {
        return ptr_.get(arena_);
    }
};

namespace mapped_ {

    template <typename T, bool HasObjectSize = MappedHasObjectSize<T>> struct ObjectCreator;

    template <typename T>
    struct ObjectCreator<T, false> {

        template <typename Arena, typename... CtrArgs>
        static auto allocate_and_construct(size_t tag_size, Arena* arena, CtrArgs&&... args)
        {
            size_t size = sizeof(T);
            auto addr = arena->template allocate_space<T>(size, tag_size);
            arena->construct(addr, std::forward<CtrArgs>(args)...);
            return addr;
        }
    };

    template <typename T>
    struct ObjectCreator<T, true> {
        template <typename Arena, typename... CtrArgs>
        static auto allocate_and_construct(size_t tag_size, Arena* arena, CtrArgs&&... args)
        {
            size_t size = T::object_size(std::forward<CtrArgs>(args)...);
            auto addr = arena->template allocate_space<T>(size, tag_size);
            arena->construct(addr, std::forward<CtrArgs>(args)...);
            return addr;
        }
    };

}


template <typename PtrHolderT_>
class MappedArenaBase {
protected:
    uint8_t* data_;
public:
    using PtrHolderT = PtrHolderT_;
    using AddressMapping = std::unordered_map<PtrHolderT, PtrHolderT>;

    template <typename T>
    using PtrT = MappedPtr<T, PtrHolderT, MappedArenaBase>;

    MMA1_NODISCARD uint8_t* data() {return data_;}
    MMA1_NODISCARD const uint8_t* data() const {return data_;}

    MMA1_NODISCARD virtual size_t allocate_space(size_t size, size_t alignment, size_t tag_size) = 0;
    MMA1_NODISCARD virtual AddressMapping make_address_mapping() {
        return AddressMapping();
    }


    template <typename T>
    MMA1_NODISCARD PtrT<T> allocate_space(size_t size, size_t tag_size = 0) noexcept
    {
        size_t alignment = alignof(T);
        return allocate_space(size, alignment, tag_size);
    }


    template <typename T, typename... CtrArgs>
    MMA1_NODISCARD PtrT<T> allocate_explicit(size_t size, CtrArgs&&... args) noexcept {
        size_t ptr = allocate_space<T>(size);
        construct<T>(ptr, std::forward<CtrArgs>(args)...);
        return ptr;
    }



    template <typename T, typename... CtrArgs>
    MMA1_NODISCARD PtrT<T> allocate(CtrArgs&&... args) noexcept {
        return mapped_::ObjectCreator<T>::allocate_and_construct(
            0, this, std::forward<CtrArgs>(args)...
        );
    }

    template <typename T, typename... CtrArgs>
    MMA1_NODISCARD PtrT<T> allocate_tagged(size_t tag_size, CtrArgs&&... args) noexcept {
        return mapped_::ObjectCreator<T>::allocate_and_construct(
            tag_size, this, std::forward<CtrArgs>(args)...
        );
    }



    template <typename T, typename... CtrArgs>
    void construct(PtrT<T> ptr, CtrArgs&&... args) noexcept
    {
        (void)new (data() + ptr.get()) T(std::forward<CtrArgs>(args)...);
    }

    bool is_null(const void* ptr) const {
        return data_ == ptr;
    }
};


template <typename T, typename PtrHolderT, typename... CtrArgs>
typename MappedArenaBase<PtrHolderT>::template PtrT<T> allocate(MappedArenaBase<PtrHolderT>* arena, CtrArgs&&... args) {
    return arena->template allocate<T>(std::forward<CtrArgs>(args)...);
}

template <typename T, typename PtrHolderT, typename... CtrArgs>
typename MappedArenaBase<PtrHolderT>::template PtrT<T> allocate_tagged(size_t tag_size, MappedArenaBase<PtrHolderT>* arena, CtrArgs&&... args) {
    return arena->template allocate_tagged<T>(tag_size, std::forward<CtrArgs>(args)...);
}


template <typename HeaderT, typename PtrHolderT>
class MappedArena: public MappedArenaBase<PtrHolderT> {
    using Base = MappedArenaBase<PtrHolderT>;

    using ArenaBufferT = ArenaBuffer<uint8_t>;
    using ArenaBufferPtr = std::unique_ptr<ArenaBufferT>;

    using Base::data_;


    size_t size_{};
    ArenaBufferPtr arena_;


public:
    using Base::data;

    using ArenaBase = Base;

    MappedArena()
    {
        create_buffer();
    }

    HeaderT& header() {
        return *T2T<HeaderT*>(data_);
    }

    const HeaderT& header() const {
        return *T2T<HeaderT*>(data_);
    }

    MappedArena(Span<uint8_t> span) noexcept:
        size_(span.size()), arena_()
    {
        data_ = span.data();
    }

    void make_writable()
    {
        if (!arena_)
        {
            uint8_t* data = data_;
            create_buffer();

            arena_->ensure(size_);

            if(MMA1_LIKELY(data_)) {
                MemCpyBuffer(data, data_, size_);
            }

            size_ = 0;
        }
    }

    void clear() noexcept
    {
        if (arena_)
        {
            HeaderT header = *T2T<const HeaderT*>(arena_->data());
            arena_->clear();

            arena_->ensure(sizeof(HeaderT));
            arena_->add_size(sizeof(HeaderT));

            *T2T<HeaderT*>(arena_->data()) = header;
        }
    }

    void reset() noexcept
    {
        if (arena_) {
            HeaderT header = *T2T<const HeaderT*>(arena_->data());
            arena_->reset();

            arena_->ensure(sizeof(HeaderT));
            arena_->add_size(sizeof(HeaderT));

            *T2T<HeaderT*>(arena_->data()) = header;
        }
    }


    virtual size_t allocate_space(size_t size, size_t alignment, size_t tag_size)
    {
        if (arena_) {
            size_t addr   = align_up(arena_->size(), alignment);
            size_t upsize = addr - arena_->size();

            while (upsize < tag_size) {
                addr += alignment;
                upsize += alignment;
            }

            arena_->ensure(upsize + size);
            arena_->add_size(upsize + size);

            return addr;
        }
        else {
            MMA1_THROW(UnmodifiableArenaException());
        }
    }

    size_t size() const {
        if (arena_) {
            return arena_->size();
        }
        else {
            return size_;
        }
    }

private:
    void create_buffer()
    {
        arena_ = std::make_unique<ArenaBufferT>(64, [&, this](int32_t buffer_id, ArenaBufferCmd cmd, size_t size, uint8_t*) -> uint8_t* {
            if (ArenaBufferCmd::ALLOCATE == cmd)
            {
                uint8_t* addr = allocate_system<uint8_t>(size).release();
                this->data_ = addr;
                return addr;
            }

            return nullptr;
        });

        arena_->ensure(sizeof(HeaderT));
        arena_->add_size(sizeof(HeaderT));

        (void)new (arena_->data()) HeaderT{};
    }

    static constexpr size_t align_up(size_t value, size_t alignment) noexcept
    {
        size_t mask = alignment - 1;
        size_t res = value & mask;

        if (MMA1_UNLIKELY(res)) {
            return (value | mask) + 1;
        }

        return value;
    }
};




}}
