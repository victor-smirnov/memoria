
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
#include <memoria/v1/core/tools/assert.hpp>

#include <typeinfo>
#include <type_traits>


namespace memoria {
namespace v1 {
namespace io {

struct IOSubstream {
    virtual ~IOSubstream() noexcept {}

    virtual void reset() = 0;
    virtual const std::type_info& substream_type() const = 0;
    virtual void init(void* ptr) = 0;
};

template <typename T>
T& substream_cast(IOSubstream& ss)
{
    static_assert(std::is_base_of<IOSubstream, T>::value, "");
    return *T2T<T*>(&ss);
}

template <typename T>
const T& substream_cast(const IOSubstream& ss)
{
    static_assert(std::is_base_of<IOSubstream, T>::value, "");
    return *T2T<const T*>(&ss);
}

template <typename T>
T& checked_substream_cast(IOSubstream& ss)
{
    static_assert(std::is_base_of<IOSubstream, T>::value, "");
    MEMORIA_V1_ASSERT_TRUE(ss.substream_type() == typeid(T));
    return *T2T<T*>(&ss);
}

template <typename T>
const T& checked_substream_cast(const IOSubstream& ss)
{
    static_assert(std::is_base_of<IOSubstream, T>::value, "");
    MEMORIA_V1_ASSERT_TRUE(ss.substream_type() == typeid(T));
    return *T2T<const T*>(&ss);
}

}}}