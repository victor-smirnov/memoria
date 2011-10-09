
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_CONTAINER_PAGES_TRAITS_HPP
#define	_MEMORIA_CORE_CONTAINER_PAGES_TRAITS_HPP

#include <memoria/core/types/typehash.hpp>
#include <memoria/core/tools/reflection.hpp>
#include <memoria/core/container/page.hpp>


namespace memoria    {


template <typename Object, size_t Size_>
class TypeHash<memoria::AbstractPageID<Object, Size_> > {
public:
    static const UInt Value = TypeHash<Object>::Value;
};

template <typename T, size_t Size>
struct FieldFactory<memoria::AbstractPageID<T, Size> > {
private:
    typedef memoria::AbstractPageID<T, Size>                                             Type;

public:
    static void create(MetadataList &list, const Type &field, const string &name, Long &abi_ptr) {
        list.push_back(new TypedIDFieldImpl<Type>(PtrToLong(&field), abi_ptr, name));
        abi_ptr += ValueTraits<Type>::Size;
    }
};


template <Int Size>
struct FieldFactory<memoria::BitBuffer<Size> > {
    typedef memoria::BitBuffer<Size>                                                     Type;

    static void create(MetadataList &list, const Type &field, const string &name, Long &abi_ptr) {
        list.push_back(new FlagFieldImpl(PtrToLong(&field), abi_ptr, name, 0, Size));
        abi_ptr += ValueTraits<Type>::Size;
    }
};

}

#endif	// _MEMORIA_CORE_CONTAINER_PAGES_HPP
