
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
#include <memoria/v1/core/types/type2type.hpp>
#include <memoria/v1/core/tools/bitmap.hpp>
#include <memoria/v1/core/tools/span.hpp>

#include <memoria/v1/core/linked/common/arena.hpp>

#include <cstddef>

namespace memoria {
namespace v1 {

template <typename T, typename Arena>
class LinkedDynVector {

    static_assert(std::is_trivially_copyable <T>::value, "");

    template <typename TT>
    using PtrT = typename Arena::template PtrT<TT>;

public:
    struct State {
        uint32_t capacity_;
        uint32_t size_;
        PtrT<T> data_;
    };
private:

    const Arena* arena_;
    PtrT<State> state_;

public:
    LinkedDynVector(): arena_(), state_({}) {}

    LinkedDynVector(const Arena* arena, PtrT<State> state):
        arena_(arena), state_(state)
    {}

    static LinkedDynVector create(Arena* arena,  uint32_t capacity)
    {
        PtrT<State> ptr = allocate<State>(arena, State{capacity, 0});
        PtrT<T> data = arena->template allocate_space<T>(capacity * sizeof(T));

        State* state = ptr.get_mutable(arena);

        state->data_ = data;

        return LinkedDynVector<T, Arena>(arena, ptr);
    }


    static LinkedDynVector create_tagged(size_t tag_size, Arena* arena, uint32_t capacity)
    {
        PtrT<State> ptr = allocate_tagged<State>(tag_size, arena, State{capacity, 0});
        PtrT<T> data = arena->template allocate_space<T>(capacity * sizeof(T));

        State* state = ptr.get(arena);
        state->data_ = data;

        return LinkedDynVector<T, Arena>(arena, ptr);
    }



    static LinkedDynVector get(const Arena* arena, PtrT<State> ptr) {
        return LinkedDynVector{arena, ptr.get()};
    }




    size_t size() const {
        return state()->size_;
    }

    size_t capacity() const {
        return state()->capacity_;
    }

    T& access(size_t idx) noexcept {
        return data_mutable()[idx];
    }

    const T& access(size_t idx) const noexcept {
        return data()[idx];
    }

    bool has_space_for(size_t size) const
    {
        const State* state = this->state();

        return (size_t)state->size_ + size <= (size_t)state->capacity_;
    }

    Span<T> span() noexcept {
        return Span<T>(data(), size());
    }

    Span<const T> span() const noexcept {
        return Span<const T>(data(), size());
    }

    void push_back(const T& value) noexcept
    {
        State* state = this->state_mutable();

        if (MMA1_LIKELY(state->size_ + 1 <= state->capacity_))
        {
            T* data = state->data_.get_mutable(arena_);
            data[state->size_++] = value;
        }
    }



    void remove(size_t idx, size_t size) noexcept
    {
        State* state = this->state_mutable();
        T* data = this->data_mutable();

        for (size_t c = idx + size; c < state->size_; c++)
        {
            data[c - size] = data[c];
        }

        for (size_t c = state->size_ - size; c < state->size_; c++)
        {
            data[c] = T{};
        }

        state->size_ -= size;
    }

    void insert(size_t idx, const T& value)
    {
        ensure(1);

        State* state = this->state_mutable();

        if (state->size_ + 1 <= state->capacity_)
        {
            state->size_ += 1;
            shift_right(idx, 1);
            T* data = this->data_mutable();
            data[idx] = value;
        }
    }

    void insert(size_t idx, Span<const T> data)
    {
        ensure(data.length());

        State* state = this->state_mutable();

        if ((size_t)state->size_ + data.size() <= (size_t)state->capacity_)
        {
            state->size_ += data.size();
            shift_right(idx, data.size());

            T* my_data = this->data_mutable();
            for (size_t c = idx; c < data.size(); c++) {
                my_data[c] = data[c - idx];
            }
        }
    }


    void clear()
    {
        State* state = this->state_mutable();
        state->size_ = 0;
        T* data = state->data_.get_mutable(arena_);

        for (size_t c = 0; c < state->capacity_; c++) {
            data[c] = T{};
        }
    }

    T* data_mutable() {
        return state_mutable()->data_.get_mutable(arena_);
    }

    const T* data() const {
        return state()->data_.get(arena_);
    }

    void ensure(size_t size)
    {
        State* state = this->state_mutable();

        if (state->size_ + size > state->capacity_)
        {
            size_t delta = state->size_ + size - state->capacity_;
            enlarge(delta);
        }
    }

    void enlarge(size_t requested)
    {
        State* state = this->state_mutable();

        size_t next_capaicty = state->capacity_ * 2;
        if (next_capaicty == 0) next_capaicty = 1;

        while (state->capacity_ + requested > next_capaicty)
        {
            next_capaicty *= 2;
        }

        PtrT<T> new_ptr = arena_->make_mutable()->template allocate_space<T>(next_capaicty * sizeof(T));

        state = this->state();

        if (state->size_ > 0)
        {
            T* buffer = state->data_.get_mutable(arena_);
            MemCpyBuffer(buffer, new_ptr.get_mutable(arena_), state->size_);
        }

        state->data_ = new_ptr;
        state->capacity_ = next_capaicty;
    }

    auto ptr() const {
        return state_.get();
    }

    template <typename Fn>
    void for_each(Fn&& fn) const
    {
        const State* state = this->state();
        const T* data = state->data_.get(arena_);

        for (uint32_t c = 0; c < state->size_; c++)
        {
           fn(data[c]);
        }

    }

private:
    State* state_mutable() noexcept {
        return state_.get_mutable(arena_);
    }

    const State* state() const noexcept {
        return state_.get(arena_);
    }


    void shift_right(size_t idx, size_t size) noexcept
    {
        State* state = this->state_mutable();
        T* data = state->data_.get_mutable(arena_);

        for (size_t c = state->size_ - size; c >= idx; c--) {
            data[c + size] = data[c];
        }
    }
};

}}
