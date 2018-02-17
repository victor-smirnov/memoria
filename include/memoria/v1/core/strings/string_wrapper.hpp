
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
#include <memoria/v1/core/types/type2type.hpp>

#include <ostream>



namespace memoria {
namespace v1 {



class StringWrapper {
public:
    using CharT     = char;
private:
    using MyType    = StringWrapper;

    const CharT* data_;
    size_t length_;

    bool owner_;

public:
    StringWrapper():
        data_(nullptr), length_(0),
        owner_(false)
    {}

    StringWrapper(const CharT* data, size_t length, bool owner = false):
        length_(length),
        owner_(owner)
    {
        if (!owner)
        {
            data_ = data;
        }
        else {
            CharT* tmp = T2T<CharT*>(malloc(length_));

            if (!tmp)
            {
                throw Exception(MA_RAW_SRC, "Can't allocate data buffer for U8String");
            }

            CopyBuffer(data, tmp, length_);

            data_ = tmp;
        }
    }

    StringWrapper(const StringWrapper& other): StringWrapper(other.data_, other.length_, true)
    {};

    StringWrapper(StringWrapper&& other):
        data_(other.data_), length_(other.length_),
        owner_(other.owner_)
    {
        other.owner_ = false;
    }

    ~StringWrapper()
    {
        if (owner_)
        {
            ::free(const_cast<CharT*>(data_));
        }
    }

    const CharT* data() const {
        return data_;
    }

    size_t length() const {
        return length_;
    }

    size_t size() const {
        return length_;
    }

    operator std::string() const
    {
        return std::string(data_, length_);
    }

    const CharT& operator[](size_t idx) const {
        return data_[idx];
    }


    MyType& operator=(const MyType& other)
    {
        if (owner_)
        {
            ::free(const_cast<CharT*>(data_));
        }

        CharT* tmp = T2T<CharT*>(malloc(other.length_));

        CopyBuffer(other.data_, tmp, other.length_);

        data_ = tmp;
        length_ = other.length_;

        owner_ = true;

        return *this;
    }

    MyType& operator=(MyType&& other)
    {
        if (owner_)
        {
            ::free(const_cast<CharT*>(data_));
        }

        data_ = other.data_;
        length_ = other.length_;

        owner_ = other.owner_;

        other.owner_ = false;

        return *this;
    }


    bool operator!=(const MyType& other) const {
        return !operator==(other);
    }


    bool operator==(const MyType& other) const
    {
        if (length_ == other.length_)
        {
            for (size_t c = 0; c < length_; c++)
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
        return std::lexicographical_compare(data_, data_ + length_, other.data_, other.data_ + other.length_);
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
        std::swap(length_, other.length_);
        std::swap(owner_, other.owner_);
    }
};


inline std::ostream& operator<<(std::ostream& out, const StringWrapper& str)
{
    out << (std::string)str;
    return out;
}

template <typename T> struct TypeHash;

template <>
struct TypeHash<StringWrapper> {
    static constexpr uint64_t Value = 2231112;
};


}}


