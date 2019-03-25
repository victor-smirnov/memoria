
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

#include <memoria/v1/core/iovector/io_substream_base.hpp>

#include <typeinfo>

namespace memoria {
namespace v1 {
namespace io {

enum class ArrayLayout {BY_COLUMN, BY_ROW};

class IORowwiseArraySubstreamBase: public IOSubstream {
protected:
    uint8_t* data_buffer_{};
    int32_t data_buffer_size_{};
    int32_t size_{};
    int32_t value_size_{1};
    int32_t alignment_{1};
    int32_t alignment_mask_{};
    int32_t data_size_{};
public:

    uint8_t* data_buffer() {return data_buffer_;}
    const uint8_t* data_buffer() const {return data_buffer_;}

    int32_t data_buffer_size() const {return data_buffer_size_;}
    int32_t data_size() const {return data_size_;}

    int32_t size() const {return size_;}

    virtual const std::type_info& content_type() const  = 0;

    virtual bool is_static() const              = 0;
    virtual uint8_t* select(int32_t idx) const  = 0;

    virtual uint8_t* reserve(int32_t data_size, int32_t values) = 0;
    virtual uint8_t* reserve(int32_t data_size, int32_t values, uint64_t* nulls_bitmap) = 0;

    virtual uint8_t* enlarge(int32_t required)  = 0;
};


struct ArrayColumnMetadata {
    uint8_t* data_buffer;
    int32_t data_size;
    int32_t data_buffer_size;
    int32_t size;
};


struct IOColumnwiseArraySubstreamBase: IOSubstream {
    virtual ArrayColumnMetadata describe(int32_t column) const  = 0;
    virtual int32_t columns() const                             = 0;

    virtual const std::type_info& content_type() const          = 0;

    virtual uint8_t* select(int32_t column, int32_t idx) const  = 0;
    virtual ArrayColumnMetadata select_and_describe(int32_t column, int32_t idx) const  = 0;

    virtual uint8_t* reserve(int32_t column, int32_t data_size, int32_t values) = 0;
    virtual uint8_t* reserve(int32_t column, int32_t data_size, int32_t values, uint64_t* nulls_bitmap) = 0;

    virtual uint8_t* reserve(int32_t column, int32_t data_size, int32_t values, int32_t* lengths) = 0;
    virtual uint8_t* reserve(int32_t column, int32_t data_size, int32_t values, int32_t* lengths, uint64_t* nulls_bitmap) = 0;


    virtual uint8_t* enlarge(int32_t column, int32_t required)  = 0;
};




class IOColumnwiseArraySubstreamUnalignedNative: public IOColumnwiseArraySubstreamBase {

public:
    IOColumnwiseArraySubstreamUnalignedNative(){}

    template <typename T>
    void set(int32_t column, int32_t pos, T&& value)
    {
        this->template access<T>(column, pos) = value;
    }


    template <typename T>
    T& access(int32_t column, int32_t pos)
    {
        static_assert(IsPackedStructV<T>, "Requested value's type must satify IsPackedStructV<>");

        auto descr = select_and_describe(column, pos);

        range_check(descr, pos, sizeof(T));

        return *T2T<T*>(descr.data_buffer);
    }

    template <typename T>
    const T& access(int32_t column, int32_t pos) const
    {
        static_assert(IsPackedStructV<T>, "Requested value's type must satify IsPackedStructV<>");

        auto descr = select_and_describe(column, pos);

        range_check(descr, pos, sizeof(T));

        return *T2T<const T*>(descr.data_buffer);
    }

    template <typename T>
    void append(int column, const T& value)
    {
        static_assert(IsPackedStructV<T>, "Requested value's type must satify IsPackedStructV<>");

        uint8_t* data_buffer = reserve(column, sizeof(T), 1);

        *T2T<T*>(data_buffer) = value;
    }

    virtual const std::type_info& substream_type() const
    {
        return typeid(IOColumnwiseArraySubstreamUnalignedNative);
    }

protected:
    void range_check(const ArrayColumnMetadata& descr, int32_t pos, int32_t value_size) const
    {
        if (MMA1_UNLIKELY(value_size > descr.data_buffer_size))
        {
            MMA1_THROW(BoundsException()) << fmt::format_ex(u"IOColumnwiseArraySubstreamUnalignedNative range check: pos={}, value size={}", pos, value_size);
        }
    }
};


using IOColumnwiseArraySubstream = IOColumnwiseArraySubstreamUnalignedNative;

template <typename T>
T& access(IOColumnwiseArraySubstreamUnalignedNative* stream, int32_t column, int32_t pos)
{
    return stream->template access<T>(column, pos);
}

template <typename T>
const T& access(const IOColumnwiseArraySubstreamUnalignedNative* stream, int32_t column, int32_t pos)
{
    return stream->template access<T>(column, pos);
}




}}}
