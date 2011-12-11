
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_REFLECTION_HPP
#define	_MEMORIA_CORE_TOOLS_REFLECTION_HPP


#include <memoria/core/vapi/api.hpp>

namespace memoria    {

using namespace memoria::vapi;

inline BigInt PtrToLong(const void *ptr) {
    return (BigInt)ptr;
}

template <typename T> struct FieldFactory;

template <typename Type>
struct FieldFactory {
    static void create(MetadataList &list, const Type &field, const string &name, Long &abi_ptr) {
        list.push_back(new MetadataGroupImpl(name, field.GetFields(abi_ptr)));
    }
};

template <typename Type>
class BitField{};

template <typename Type>
struct FieldFactory<BitField<Type> > {
    static void create(MetadataList &list, const Type &field, const string &name, Long offset, Long &abi_ptr) {
        list.push_back(new FlagFieldImpl(PtrToLong(&field), abi_ptr, name, offset));
    }

    static void create(MetadataList &list, const Type &field, const string &name, Long offset, Long count, Long &abi_ptr) {
        list.push_back(new FlagFieldImpl(PtrToLong(&field), abi_ptr, name, offset, count));
    }
};

template <>
struct FieldFactory<EmptyValue> {
	static void create(MetadataList &list, const EmptyValue &field, const string &name, Long &abi_ptr) {
	}
};



#define MEMORIA_TYPED_FIELD(Type)                                                \
template <> struct FieldFactory<Type> {                                         \
    static void create(MetadataList &list, const Type &field, const string &name, Long &abi_ptr) {\
        list.push_back(new TypedFieldImpl<Type>((Int)PtrToLong(&field), abi_ptr, name)); \
        abi_ptr += (Long)sizeof(Type);                                          \
    }                                                                           \
                                                                                \
    static void create(MetadataList &list, const Type &field, const string &name, Long size, Long &abi_ptr) {\
        list.push_back(new TypedFieldImpl<Type>((Int)PtrToLong(&field), abi_ptr, name, size)); \
        abi_ptr += size * (Long)sizeof(Type);                                   \
    }                                                                           \
}


MEMORIA_TYPED_FIELD(Byte);
MEMORIA_TYPED_FIELD(Short);
MEMORIA_TYPED_FIELD(Int);
MEMORIA_TYPED_FIELD(BigInt);
MEMORIA_TYPED_FIELD(UByte);
MEMORIA_TYPED_FIELD(UShort);
MEMORIA_TYPED_FIELD(UInt);


#undef MEMORIA_TYPED_FIELD


}

#endif	/* _MEMORIA_CORE_REFLECTION_HPP */
