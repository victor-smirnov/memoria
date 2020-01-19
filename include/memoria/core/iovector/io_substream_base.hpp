
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

#include <memoria/core/types.hpp>
#include <memoria/core/types/type2type.hpp>

#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/strings/format.hpp>
#include <memoria/core/tools/assert.hpp>

#include <memoria/core/tools/span.hpp>
#include <memoria/core/tools/arena_buffer.hpp>
#include <memoria/core/tools/vle_arena_buffer.hpp>
#include <memoria/core/datatypes/buffer/buffer_common.hpp>

#include <typeinfo>
#include <type_traits>


namespace memoria {
namespace io {

struct IOSubstream {
    virtual ~IOSubstream() noexcept {}

    virtual void reset()   = 0;
    virtual void clear() {
        reset();
    }

    virtual void reindex() = 0;

    virtual U8String describe() const = 0;

    virtual const std::type_info& substream_type() const = 0;
};


template <typename DataType>
struct IO1DArraySubstreamView: IOSubstream {

    using ViewType = DTTViewType<DataType>;

    virtual void reset() {}
    virtual void reindex() {}

    virtual size_t size() const = 0;

    virtual void read_to(size_t row, size_t size, ArenaBuffer<ViewType>& buffer) const = 0;
    virtual void read_to(size_t row, size_t size, DataTypeBuffer<DataType>& buffer) const = 0;
    virtual void read_to(size_t row, size_t size, Span<ViewType> buffer) const = 0;

    virtual Span<const ViewType> span(size_t row, size_t size) const = 0;

    virtual ViewType get(size_t row) const = 0;

    virtual const std::type_info& substream_type() const {
        return typeid(IO1DArraySubstreamView<DataType>);
    }
};

template <typename T>
T& substream_cast(IOSubstream& ss)
{
    static_assert(std::is_base_of<IOSubstream, T>::value, "");
    return *static_cast<T*>(&ss);
}

template <typename T>
const T& substream_cast(const IOSubstream& ss)
{
    static_assert(std::is_base_of<IOSubstream, T>::value, "");
    return *static_cast<const T*>(&ss);
}

template <typename T>
T& checked_substream_cast(IOSubstream& ss)
{
    static_assert(std::is_base_of<IOSubstream, T>::value, "");
    MEMORIA_V1_ASSERT_TRUE(ss.substream_type() == typeid(T));
    return *static_cast<T*>(&ss);
}

template <typename T>
const T& checked_substream_cast(const IOSubstream& ss)
{
    static_assert(std::is_base_of<IOSubstream, T>::value, "");
    MEMORIA_V1_ASSERT_TRUE(ss.substream_type() == typeid(T));
    return *static_cast<const T*>(&ss);
}



}

template <typename DataType, typename ArraySO>
class IO1DArraySubstreamViewImpl;

}
