
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TYPES_TYPE2TYPE_HPP
#define _MEMORIA_CORE_TYPES_TYPE2TYPE_HPP

#include <memoria/core/types/types.hpp>

namespace memoria    {

template <typename SrcType, typename DstType>
union T2TBuf {
    SrcType     src;
    DstType     dst;
    T2TBuf(SrcType src0): src(src0) {}
};


template <typename DstType, typename SrcType>
DstType T2T(SrcType value) {
    T2TBuf<SrcType, DstType> buf(value);
    return buf.dst;
}

template <typename DstType, typename SrcType>
DstType T2T_S(SrcType value) {
    return static_cast<DstType>(value);
}

template <typename DstType, typename SrcType>
DstType P2V(SrcType value) {
    T2TBuf<SrcType, DstType*> buf(value);
    return *buf.dst;
}

template <typename DstType, typename SrcType>
const DstType* CP2CP(const SrcType* value) {
    T2TBuf<const SrcType*, const DstType*> buf(value);
    return buf.dst;
}

template <typename DstType, typename SrcType>
DstType& P2R(SrcType value) {
    T2TBuf<SrcType, DstType*> buf(value);
    return *buf.dst;
}

template <typename DstType, typename SrcType>
const DstType& P2CR(const SrcType value) {
    T2TBuf<const SrcType, const DstType*> buf(value);
    return *buf.dst;
}

}

#endif



