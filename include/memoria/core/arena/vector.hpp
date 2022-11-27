
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

#include <memoria/core/arena/arena.hpp>
#include <memoria/core/arena/relative_ptr.hpp>

#include <memoria/core/tools/span.hpp>
#include <memoria/core/reflection/reflection.hpp>


namespace memoria {
namespace arena {

namespace detail {

template <typename T>
struct CopyHelper {
    static void copy(T& dst, const T& src) {
        dst = src;
    }

    static void set_default(T& dst) {
        dst = T{};
    }
};

template <typename T>
struct CopyHelper<RelativePtr<T>> {
    static void copy(RelativePtr<T>& dst, const RelativePtr<T>& src) {
        dst = src.get();
    }

    static void set_default(RelativePtr<T>& dst) {
        dst = nullptr;
    }
};

template <typename T>
struct CopyHelper<EmbeddingRelativePtr<T>> {
    static void copy(ERelativePtr& dst, const EmbeddingRelativePtr<T>& src) {
        if (src.is_pointer()) {
            dst = src.get();
        }
        else {
            dst.copy_from(src);
        }
    }

    static void set_default(EmbeddingRelativePtr<T>& dst) {
        dst = static_cast<void*>(0);
    }
};

}

template <typename T>
class Vector {
    uint64_t size_;
    uint64_t capacity_;
    RelativePtr<T> data_;
public:
    Vector() noexcept :
        size_(), capacity_()
    {}

    uint64_t size() const {return size_;}

    uint64_t capacity() const {
        return capacity_;
    }

    bool has_extra_space() const {
        return capacity() > size_;
    }

    const T& get(size_t idx) const {
        return data_.get()[idx];
    }

    T& get(size_t idx) {
        return data_.get()[idx];
    }

    void set(size_t idx, const T& value)
    {
        detail::CopyHelper<T>::copy(data_.get()[idx], value);
    }

    void push_back(ArenaAllocator& arena, const T& value)
    {
        size_t cc = capacity();
        if (size_ >= cc) {
            enlarge(arena, cc + 1);
        }

        set(size_, value);
        size_++;
    }

    template <typename Fn>
    void push_back(ArenaAllocator& arena, Fn&& provider)
    {
        size_t cc = capacity();
        if (size_ >= cc) {
            enlarge(arena, cc + 1);
        }

        set(size_, std::move(provider));
        size_++;
    }

    void remove(ArenaAllocator& arena, uint64_t idx)
    {
        T* data = data_.get();
        for (size_t c = idx + 1; c < size_; c++) {
            detail::CopyHelper<T>::copy(data[c - 1], data[c]);
        }

        detail::CopyHelper<T>::set_default(data[size_ - 1]);
        --size_;
    }

    void insert(ArenaAllocator& arena, uint64_t idx, const T& val)
    {
        ensure(1);
        ++size_;

        T* data = data_.get();
        for (size_t c = size_; c > idx; c--)
        {
            detail::CopyHelper<T>::copy(data[c], data[c - 1]);
        }

        detail::CopyHelper<T>::copy(data[idx], val);
    }

    Span<const T> span() const {
        return Span<const T>(data_.get(), size_);
    }

    Span<T> span() {
        return Span<T>(data_.get(), size_);
    }


    void ensure(ArenaAllocator& arena, size_t size)
    {
        size_t room_extra = capacity_ - size_;
        if (size > room_extra) {
            enlarge(arena, size - room_extra);
        }
    }

    void enlarge(ArenaAllocator& arena, size_t target_size)
    {
        size_t cc = capacity_ > 0 ? capacity_ : 2;
        while (cc < target_size) {
            cc *= 2;
        }

        auto new_data = arena.template allocate_untagged_array<T>(cc);

        T* data = data_.get();

        for (size_t c = 0; c < size_; c++) {
            detail::CopyHelper<T>::copy(new_data[c], data[c]);
        }

        data_ = new_data;
        capacity_ = cc;
    }


    Vector* deep_copy_to(
            ArenaAllocator& dst,
            ShortTypeCode tag,
            void* owner_view,
            ViewPtrHolder* ptr_holder,
            DeepCopyDeduplicator& dedup) const
    {
        Vector* existing = dedup.resolve(dst, this);
        if (MMA_LIKELY((bool)existing)) {
            return existing;
        }
        else {
            // FIXME: shrink the array if it's small here
            auto vv = dst.get_resolver_for(dst.template allocate_tagged_object<Vector>(tag));
            dedup.map(dst, this, vv.get(dst));

            vv.get(dst)->size_ = size_;
            vv.get(dst)->capacity_ = capacity_;

            if (data_.is_not_null())
            {
                auto data = dst.get_resolver_for(dst.template allocate_untagged_array<T>(capacity_));

                vv.get(dst)->data_ = data.get(dst);
                const T* src_data =  data_.get();

                memoria::detail::DeepCopyHelper<T>::deep_copy_to(dst, data, src_data, size_, owner_view, ptr_holder, dedup);
            }

            return vv.get(dst);
        }
    }
};



}}
