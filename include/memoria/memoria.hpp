
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_HPP
#define _MEMORIA_HPP

#include <memoria/core/tools/file.hpp>
#include <memoria/allocators/inmem/factory.hpp>

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
struct CtrNameDeclarator<7>: TypeDef<VectorMap<BigInt, BigInt>> {};

template <>
struct CtrNameDeclarator<8>: TypeDef<Sequence<1, true>> {};

template <>
struct CtrNameDeclarator<9>: TypeDef<Sequence<8, true>> {};


template <>
struct CtrNameDeclarator<16>: TypeDef<LabeledTree<FLabel<UByte>, VLabel<BigInt, Granularity::Bit, Indexed::Yes>>> {};

template <>
struct CtrNameDeclarator<18>: TypeDef<WT> {};

template <>
struct CtrNameDeclarator<19>: TypeDef<VTree> {};


}

#endif


