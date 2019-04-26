
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

#include <memoria/v1/core/tools/type_name.hpp>

#include <typeinfo>

namespace memoria {
namespace v1 {
namespace io {


template<typename Value>
struct IORowwiseVLenArraySubstream: IOSubstream {
    virtual int32_t columns() const                     = 0;

    virtual const uint8_t* select(int32_t row) const    = 0;
    virtual uint8_t* select(int32_t row)                = 0;

    virtual void get_lengths_to(int32_t start, int32_t size, int32_t* array) const = 0;

    virtual uint8_t* reserve(int32_t rows, const int32_t* lengths) = 0;    
    virtual uint8_t* ensure(int32_t capacity) = 0;

    virtual U8String describe() const {
        return TypeNameFactory<IORowwiseVLenArraySubstream<Value>>::name().to_u8();
    }

    virtual const std::type_info& substream_type() const {
        return typeid(IORowwiseVLenArraySubstream<Value>);
    }
};



struct VLenArrayColumnMetadata {
    uint8_t* data_buffer;
    int32_t data_size;
    int32_t data_buffer_size;
    int32_t size;
};

struct ConstVLenArrayColumnMetadata {
    const uint8_t* data_buffer;
    int32_t data_size;
    int32_t data_buffer_size;
    int32_t size;
};

template <typename Value>
struct IOColumnwiseVLenArraySubstream: IOSubstream {
    virtual ConstVLenArrayColumnMetadata describe(int32_t column) const    = 0;
    virtual VLenArrayColumnMetadata describe(int32_t column)               = 0;


    virtual int32_t columns() const                                   = 0;
    virtual const uint8_t* select(int32_t column, int32_t idx) const  = 0;
    virtual uint8_t* select(int32_t column, int32_t idx)              = 0;

    virtual void get_lengths_to(int32_t column, int32_t start, int32_t size, int32_t* array) const = 0;

    virtual VLenArrayColumnMetadata select_and_describe(int32_t column, int32_t idx)              = 0;
    virtual ConstVLenArrayColumnMetadata select_and_describe(int32_t column, int32_t idx) const   = 0;

    virtual uint8_t* reserve(int32_t column, int32_t size, const int32_t* lengths) = 0;

    virtual uint8_t* ensure(int32_t column, int32_t capacity) = 0;

    virtual U8String describe() const {
        return TypeNameFactory<IOColumnwiseVLenArraySubstream<Value>>::name().to_u8();
    }


    virtual const std::type_info& substream_type() const {
        return typeid(IOColumnwiseVLenArraySubstream<Value>);
    }

    virtual const std::type_info& content_type() const {
        return typeid(Value);
    }
};

}}}
