
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

#include <memoria/v1/core/exceptions/exceptions.hpp>
#include <memoria/v1/core/strings/format.hpp>

#include <typeinfo>

namespace memoria {
namespace v1 {
namespace io {

class IOSubstreamBase {
protected:
    uint8_t* data_buffer_{};
    int32_t data_buffer_size_{};
    int32_t size_{};
    int32_t value_size_{1};
    int32_t alignment_{1};
    int32_t alignment_mask_{};
    int32_t data_size_{};
public:
    virtual ~IOSubstreamBase() noexcept {}

    uint8_t* data_buffer() {return data_buffer_;}
    const uint8_t* data_buffer() const {return data_buffer_;}

    int32_t data_buffer_size() const {return data_buffer_size_;}
    int32_t data_size() const {return data_size_;}

    int32_t size() const {return size_;}

    virtual const std::type_info& type() const  = 0;

    virtual bool is_static() const              = 0;
    virtual uint8_t* select(int32_t idx) const  = 0;

    virtual uint8_t* reserve(int32_t data_size, int32_t values) = 0;
    virtual uint8_t* reserve(int32_t data_size, int32_t values, uint64_t* nulls_bitmap) = 0;

    virtual uint8_t* enlarge(int32_t required)  = 0;
    virtual void reset()                        = 0;
};




class IOSubstreamUnalignedNative: public IOSubstreamBase {

public:
    IOSubstreamUnalignedNative(){}

    template <typename T>
    void set(int32_t pos, T&& value)
    {
        this->template access<T>(pos) = value;
    }


    template <typename T>
    T& access(int32_t pos)
    {
        range_check(pos, sizeof(T));
        static_assert(IsPackedStructV<T>, "Requested value's type must satify IsPackedStructV<>");
        return *T2T<T*>(data_buffer_ + pos);
    }

    template <typename T>
    const T& access(int32_t pos) const
    {
        range_check(pos, sizeof(T));
        static_assert(IsPackedStructV<T>, "Requested value's type must satify IsPackedStructV<>");
        return *T2T<T*>(data_buffer_ + pos);
    }

    template <typename T>
    void append(const T& value)
    {
        if (MMA1_UNLIKELY(data_size_ + sizeof(T) > data_buffer_size_))
        {
            enlarge(sizeof(T));
        }

        *T2T<T*>(data_buffer_ + data_size_) = value;

        size_ += 1;
        data_size_ += sizeof(T);
    }


    template <typename T>
    void append_unchecked(const T& value)
    {
        *T2T<T*>(data_buffer_ + data_size_) = value;

        size_ += 1;
        data_size_ += sizeof(T);
    }

    void advance(int32_t data_size, int32_t size)
    {
        data_size_ += data_size;
        size_ += size;
    }

protected:
    void range_check(int32_t pos, int32_t size) const
    {
        if (MMA1_UNLIKELY(pos + size > size_))
        {
            MMA1_THROW(BoundsException()) << fmt::format_ex(u"IOStreamUnalignedNative range check: pos={}, size={}, stream size={}", pos, size, size_);
        }
    }
};

using IOSubstream = IOSubstreamUnalignedNative;


template <typename T>
T& access(IOSubstreamUnalignedNative* stream, int32_t pos)
{
    return stream->template access<T>(pos);
}

template <typename T>
const T& access(const IOSubstreamUnalignedNative* stream, int32_t pos)
{
    return stream->template access<T>(pos);
}




}}}
