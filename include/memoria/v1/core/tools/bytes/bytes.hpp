
// Copyright 2016 Victor Smirnov
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
#include <memoria/v1/core/tools/bitmap.hpp>
#include <memoria/v1/core/types/typehash.hpp>

#include <memory>
#include <algorithm>
#include <iomanip>

#include <stdlib.h>

namespace memoria {
namespace v1 {

template <typename T>
class RawData {

    using MyType = RawData<T>;

    T* data_;
    size_t size_;
    bool owner_;
public:
    RawData(): data_(nullptr), size_(0), owner_(false) {}
    explicit RawData(size_t size): data_(alloc(size)), size_(size), owner_(true) {}

    RawData(const T* data, size_t size):
        data_(alloc(size)), size_(size), owner_(true)
    {
        CopyBuffer(data, data_, size);
    }

    RawData(T* data, size_t size, bool owner):
        size_(size), owner_(owner)
    {
        if (!owner)
        {
            data_ = data;
        }
        else {
            data_ = alloc(size);
            CopyBuffer(data, data_, size);
        }
    }

    RawData(const MyType& other): data_(alloc(other.size_)), size_(other.size_), owner_(true)
    {
        CopyBuffer(other.data_, data_, size_);
    }

    RawData(MyType&& other)
    {
        data_ = other.data_;
        size_ = other.size_;
        owner_ = other.owner_;

        other.data_ = nullptr;
        other.owner_ = false;
    }

    ~RawData()
    {
        if (owner_ && data_)
        {
            ::free(data_);
        }
    }

    T* data() {return data_;}
    const T* data() const {return data_;}

    T* take()
    {
        owner_ = false;
        T* data = data_;
        data_ = nullptr;

        return data;
    }

    size_t size() const {
        return size_;
    }

    size_t length() const {
        return size_;
    }

    MyType& operator=(const MyType& other)
    {
        if (data_ && owner_) ::free(data_);

        data_ = alloc(other.size_);
        size_ = other.size_;
        owner_ = true;

        CopyBuffer(other.data_, data_, size_);

        return *this;
    }

    MyType& operator=(MyType&& other)
    {
        if (owner_ && data_) ::free(data_);

        data_ = other.data_;
        size_ = other.size_;
        owner_ = other.owner_;

        other.data_ = nullptr;
        other.owner_ = false;

        return *this;
    }


    bool operator!=(const MyType& other) const {
        return !operator==(other);
    }


    bool operator==(const MyType& other) const
    {
        if (size_ == other.size_)
        {
            for (size_t c = 0; c < size_; c++)
            {
                if (data_[c] != other.data_[c])
                {
                    return false;
                }
            }

            return true;
        }

        return false;
    }

    bool operator<(const MyType& other) const
    {
        return std::lexicographical_compare(data_, data_ + size_, other.data_, other.data_ + other.size_);
    }

    bool operator<=(const MyType& other) const
    {
        return operator<(other) || operator==(other);
    }

    bool operator>(const MyType& other) const
    {
        return !operator<=(other);
    }

    bool operator>=(const MyType& other) const
    {
        return !operator<(other);
    }

    void swap(MyType& other)
    {
        std::swap(data_, other.data_);
        std::swap(size_, other.size_);
        std::swap(owner_, other.owner_);
    }

    T& operator[](size_t c) {
        return data_[c];
    }

    const T& operator[](size_t c) const {
        return data_[c];
    }

private:
    static T* alloc(size_t size)
    {
        T* data = T2T<T*>(::malloc(size));
        if (!data)
        {
            throw Exception(MA_RAW_SRC, "Can't allocate raw data buffer");
        }
        return data;
    }
};

template <typename T>
struct TypeHash;

template <typename T>
struct TypeHash<RawData<T>> {
    static const uint32_t Value = HashHelper<99992, 22211, TypeHash<T>::Value>::Value;
};

template <typename T>
std::ostream& operator<<(std::ostream& out, const RawData<T>& val)
{
    if (val.size() > 0)
    {
        std::ios  state(nullptr);
        state.copyfmt(out);

        out<<std::setbase(16);
        for (size_t c = 0; c < val.size(); c++)
        {
            out<<std::setw(sizeof(T) * 2)<<setfill('0');
            out << (uint64_t)val[c];
        }

        out.copyfmt(state);
    }

    return out;
}

using Bytes = RawData<uint8_t>;



}}
