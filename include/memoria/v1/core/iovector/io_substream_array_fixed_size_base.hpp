
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

template <typename Value>
struct IORowwiseFixedSizeArraySubstream: IOSubstream {
    virtual int32_t columns() const                   = 0;
    virtual int32_t size() const                      = 0;


    virtual Value* select(int32_t idx)                = 0;
    virtual const Value* select(int32_t idx) const    = 0;

    virtual Value* reserve(int32_t rows)              = 0;
    virtual Value* ensure(int32_t capacity)           = 0;

    virtual U8String describe() const {
        return TypeNameFactory<IORowwiseFixedSizeArraySubstream<Value>>::name().to_u8();
    }

    virtual const std::type_info& substream_type() const {
        return typeid(IORowwiseFixedSizeArraySubstream<Value>);
    }
};




template<typename Value>
struct FixedSizeArrayColumnMetadata {
    Value* data_buffer;
    int32_t capacity;
    int32_t size;

    int32_t free_space() const {
        return capacity - size;
    }
};

template <typename Value>
struct IOColumnwiseFixedSizeArraySubstream: IOSubstream {

    virtual FixedSizeArrayColumnMetadata<Value> describe(int32_t column) = 0;
    virtual FixedSizeArrayColumnMetadata<const Value> describe(int32_t column) const  = 0;

    virtual int32_t columns() const                                   = 0;

    virtual Value* select(int32_t column, int32_t idx)                = 0;
    virtual const Value* select(int32_t column, int32_t idx) const    = 0;

    virtual Value* reserve(int32_t column, int32_t size) = 0;

    virtual Value* ensure(int32_t column, int32_t capacity) = 0;

    virtual void append(int32_t column, const Value& value) = 0;

    virtual U8String describe() const {
        return TypeNameFactory<IOColumnwiseFixedSizeArraySubstream<Value>>::name().to_u8();
    }

    virtual const std::type_info& substream_type() const {
        return typeid(IOColumnwiseFixedSizeArraySubstream<Value>);
    }
};




}}}
