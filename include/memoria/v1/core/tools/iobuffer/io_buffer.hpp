
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
#include <memoria/v1/core/exceptions/exceptions.hpp>

#include <memoria/v1/core/tools/bytes/bytes_codec.hpp>
#include <memoria/v1/core/tools/bignum/int64_codec.hpp>
#include <memoria/v1/core/tools/strings/string_codec.hpp>
#include <memoria/v1/core/tools/bitmap.hpp>
#include <memoria/v1/core/tools/uuid.hpp>

#include "io_buffer_little_aligned.hpp"
#include "io_buffer_little_unaligned.hpp"




#include <limits>
#include <tuple>

#include <malloc.h>

namespace memoria {
namespace v1 {

using DefaultIOBuffer = IOBuffer<ByteOrder::LITTLE, MemoryAccess::MMA_UNALIGNED>;
using AlignedIOBuffer = IOBuffer<ByteOrder::LITTLE, MemoryAccess::MMA_ALIGNED>;


template <typename T>
struct IOBufferAdapterBase {
    template <typename IOBuffer>
    static bool put(IOBuffer& buffer, const T& value) {
        return buffer.put(value);
    }
};

template <>
struct IOBufferAdapter<char>: IOBufferAdapterBase<char> {
    template <typename IOBuffer>
    static char get(IOBuffer& buffer) {
        return buffer.getChar();
    }
};

template <>
struct IOBufferAdapter<int8_t>: IOBufferAdapterBase<int8_t> {
    template <typename IOBuffer>
    static int8_t get(IOBuffer& buffer) {
        return buffer.getByte();
    }
};

template <>
struct IOBufferAdapter<uint8_t>: IOBufferAdapterBase<uint8_t> {
    template <typename IOBuffer>
    static uint8_t get(IOBuffer& buffer) {
        return buffer.getUByte();
    }
};

template <>
struct IOBufferAdapter<int16_t>: IOBufferAdapterBase<int16_t> {
    template <typename IOBuffer>
    static int16_t get(IOBuffer& buffer) {
        return buffer.getShort();
    }
};

template <>
struct IOBufferAdapter<uint16_t>: IOBufferAdapterBase<uint16_t> {
    template <typename IOBuffer>
    static uint16_t get(IOBuffer& buffer) {
        return buffer.getUShort();
    }
};

template <>
struct IOBufferAdapter<int32_t>: IOBufferAdapterBase<int32_t> {
    template <typename IOBuffer>
    static int32_t get(IOBuffer& buffer) {
        return buffer.getInt();
    }
};

template <>
struct IOBufferAdapter<uint32_t>: IOBufferAdapterBase<uint32_t> {
    template <typename IOBuffer>
    static uint32_t get(IOBuffer& buffer) {
        return buffer.getUInt();
    }
};


template <>
struct IOBufferAdapter<int64_t>: IOBufferAdapterBase<int64_t> {
    template <typename IOBuffer>
    static int64_t get(IOBuffer& buffer) {
        return buffer.getBigInt();
    }
};

template <>
struct IOBufferAdapter<uint64_t>: IOBufferAdapterBase<uint64_t> {
    template <typename IOBuffer>
    static uint64_t get(IOBuffer& buffer) {
        return buffer.getUBigInt();
    }
};

template <>
struct IOBufferAdapter<float>: IOBufferAdapterBase<float> {
    template <typename IOBuffer>
    static float get(IOBuffer& buffer) {
        return buffer.getFloat();
    }
};


template <>
struct IOBufferAdapter<double>: IOBufferAdapterBase<double> {
    template <typename IOBuffer>
    static double get(IOBuffer& buffer) {
        return buffer.getDouble();
    }
};

template <>
struct IOBufferAdapter<bool>: IOBufferAdapterBase<bool> {
    template <typename IOBuffer>
    static double get(IOBuffer& buffer) {
        return buffer.getBool();
    }
};

template <>
struct IOBufferAdapter<Bytes>: IOBufferAdapterBase<Bytes> {
    template <typename IOBuffer>
    static Bytes get(IOBuffer& buffer) {
        return buffer.getBytes();
    }
};

template <>
struct IOBufferAdapter<String>: IOBufferAdapterBase<String> {
    template <typename IOBuffer>
    static String get(IOBuffer& buffer) {
        return buffer.getString();
    }
};

template <>
struct IOBufferAdapter<UUID> {

    template <typename IOBuffer>
    static bool put(IOBuffer& buffer, const UUID& value)
    {
        if (buffer.put(value.hi()))
        {
            if (buffer.put(value.lo())) {
                return true;
            }
        }

        return false;
    }

    template <typename IOBuffer>
    static UUID get(IOBuffer& buffer)
    {
        return UUID(buffer.getUBigInt(), buffer.getUBigInt());
    }
};

namespace details00 {
    
    template <int32_t Idx, int32_t Max> 
    struct IOBufferTupleSetter {
        template <typename IOBuffer, typename T>
        static bool put(IOBuffer& buffer, const T& tuple)
        {
            using TT = std::remove_reference_t<std::tuple_element_t<Idx, T>>;
            
            if (IOBufferAdapter<TT>::put(buffer, std::get<Idx>(tuple))) 
            {
                return IOBufferTupleSetter<Idx + 1, Max>::put(buffer, tuple);
            }

            return false;
        }
    };
    
    
    template <int32_t Max> 
    struct IOBufferTupleSetter<Max, Max> {
        template <typename IOBuffer, typename T>
        static bool put(IOBuffer& buffer, const T& tuple)
        {
            using TT = std::remove_reference_t<std::tuple_element_t<Max, T>>;
            return IOBufferAdapter<TT>::put(buffer, std::get<Max>(tuple));
        }
    };
    
    
    template <int32_t Idx, int32_t Max> 
    struct IOBufferTupleGetter {
        template <typename IOBuffer, typename T>
        static void get(IOBuffer& buffer, T& tuple)
        {
            using TT = std::remove_reference_t<std::tuple_element_t<Idx, T>>;
            
            std::get<Idx>(tuple) = std::move(IOBufferAdapter<TT>::get(buffer));
            IOBufferTupleGetter<Idx + 1, Max>::get(buffer, tuple);
        }
    };
    
    template <int32_t Max> 
    struct IOBufferTupleGetter<Max, Max> {
        template <typename IOBuffer, typename T>
        static void get(IOBuffer& buffer, T& tuple)
        {
            using TT = std::remove_reference_t<std::tuple_element_t<Max, T>>;
            std::get<Max>(tuple) = std::move(IOBufferAdapter<TT>::get(buffer));
        }
    };
    
}

template <typename... T>
struct IOBufferAdapter<std::tuple<T...>> {
    template <typename IOBuffer>
    static bool put(IOBuffer& buffer, const std::tuple<T...>& value)
    {
        return details00::IOBufferTupleSetter<
            0, 
            sizeof...(T) - 1
        >::put(buffer, value);
    }
    
    template <typename IOBuffer>
    static auto get(IOBuffer& buffer)
    {
        std::tuple<T...> tuple;
        
        details00::IOBufferTupleGetter<
            0, 
            sizeof...(T) - 1
        >::get(buffer, tuple);
        
        return tuple;
    }
};

namespace bt {


template <typename IOBuffer>
struct BufferConsumer {
    virtual int32_t process(IOBuffer& buffer, int32_t entries) = 0;

    virtual ~BufferConsumer() noexcept {}
};

template <typename IOBuffer>
struct BufferProducer {
    virtual int32_t populate(IOBuffer& buffer) = 0;

    virtual ~BufferProducer() noexcept {}
};

}


template <typename IOBuffer, typename Fn>
class BufferFnConsumer: public bt::BufferConsumer<IOBuffer> {
    Fn fn_;
public:
    BufferFnConsumer(Fn fn): fn_(fn) {}
    
    virtual int32_t process(IOBuffer& buffer, int32_t entries) {
        return fn_(buffer, entries);
    }
};

template <typename IOBuffer, typename Fn>
class BufferFnProducer: public bt::BufferProducer<IOBuffer> {
    Fn fn_;
public:
    BufferFnProducer(Fn fn): fn_(fn) {}
    
    virtual int32_t populate(IOBuffer& buffer) {
        return fn_(buffer);
    }
};


}
}
