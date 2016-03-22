
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/core/types/typehash.hpp>
#include <memoria/core/tools/reflection.hpp>
#include <memoria/core/container/page.hpp>


namespace memoria    {

template <typename T> struct FieldFactory;



template <typename T>
struct FieldFactory<PageID<T> > {
private:
    using Type = PageID<T>;

public:
    static void serialize(SerializationData& data, const Type& field, Int count = 1)
    {
        memmove(data.buf, &field, sizeof(Type) * count);

        data.buf    += sizeof(Type) * count;
        data.total  += sizeof(Type) * count;
    }

    static void serialize(SerializationData& data, const Type* field, Int count = 1)
    {
        memmove(data.buf, field, sizeof(Type) * count);

        data.buf    += sizeof(Type) * count;
        data.total  += sizeof(Type) * count;
    }

    static void deserialize(DeserializationData& data, Type& field, Int count = 1)
    {
        memmove(&field, data.buf, sizeof(Type) * count);
        data.buf += sizeof(Type) * count;
    }

    static void deserialize(DeserializationData& data, Type* field, Int count = 1)
    {
        memmove(field, data.buf, sizeof(Type) * count);
        data.buf += sizeof(Type) * count;
    }
};


template <Int Size>
struct FieldFactory<BitBuffer<Size> > {
    using Type = BitBuffer<Size>;

    static void serialize(SerializationData& data, const Type& field)
    {
        memmove(data.buf, &field, sizeof(Type));
        data.buf    += sizeof(Type);
        data.total  += sizeof(Type);
    }

    static void deserialize(DeserializationData& data, Type& field)
    {
        memmove(&field, data.buf, sizeof(Type));
        data.buf += sizeof(Type);
    }
};

}
