
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_CONTAINER_PAGES_TRAITS11_HPP
#define _MEMORIA_CORE_CONTAINER_PAGES_TRAITS11_HPP

#include <memoria/core/types/typehash.hpp>
#include <memoria/core/tools/reflection.hpp>
#include <memoria/core/container/page.hpp>


namespace memoria    {

template <typename T> struct FieldFactory;


//template <typename Object>
//struct TypeHash<PageID<Object> > {
//public:
//    static const UInt Value = TypeHash<Object>::Value;
//};

template <typename T>
struct FieldFactory<PageID<T> > {
private:
    typedef PageID<T>                                                   Type;

public:
    static void serialize(SerializationData& data, const Type& field, Int count = 1)
    {
        memmove(data.buf, &field, sizeof(Type) * count);
        data.buf += sizeof(Type) * count;
    }

    static void deserialize(DeserializationData& data, Type& field, Int count = 1)
    {
        memmove(&field, data.buf, sizeof(Type) * count);
        data.buf += sizeof(Type) * count;
    }
};


template <Int Size>
struct FieldFactory<BitBuffer<Size> > {
    typedef BitBuffer<Size>                                                     Type;

    static void serialize(SerializationData& data, const Type& field)
    {
        memmove(data.buf, &field, sizeof(Type));
        data.buf += sizeof(Type);
    }

    static void deserialize(DeserializationData& data, Type& field)
    {
        memmove(&field, data.buf, sizeof(Type));
        data.buf += sizeof(Type);
    }
};

}

#endif  // _MEMORIA_CORE_CONTAINER_PAGES_HPP
