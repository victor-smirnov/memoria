
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_HPP
#define _MEMORIA_HPP

#include <memoria/core/tools/file.hpp>
#include <memoria/allocators/inmem/factory.hpp>
#include <memoria/core/tools/symbol_sequence.hpp>

namespace memoria {

template <>
struct CtrNameDeclarator<0>: TypeDef<Root> {};

template <>
struct CtrNameDeclarator<1>: TypeDef<Vector<Byte>> {};

template <>
struct CtrNameDeclarator<2>: TypeDef<Vector<UByte>> {};

template <>
struct CtrNameDeclarator<3>: TypeDef<Vector<Int>> {};

template <>
struct CtrNameDeclarator<4>: TypeDef<Vector<BigInt>> {};

template <>
struct CtrNameDeclarator<5>: TypeDef<Map<BigInt, BigInt>> {};

//template <>
//struct CtrNameDeclarator<6>: TypeDef<Set<BigInt>> {};

template <>
struct CtrNameDeclarator<7>: TypeDef<VectorMap<BigInt, BigInt>> {};

//template <>
//struct CtrNameDeclarator<8>: TypeDef<Sequence<1, true>> {};
//
//template <>
//struct CtrNameDeclarator<9>: TypeDef<Sequence<2, true>> {};
//
//template <>
//struct CtrNameDeclarator<10>: TypeDef<Sequence<3, true>> {};
//
//template <>
//struct CtrNameDeclarator<11>: TypeDef<Sequence<4, true>> {};
//
//template <>
//struct CtrNameDeclarator<15>: TypeDef<LOUDS> {};

template <>
struct CtrNameDeclarator<16>: TypeDef<CMap<BigInt, BigInt>> {};


}

#endif


