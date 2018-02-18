
// Copyright 2011 Victor Smirnov
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

namespace memoria {
namespace v1 {

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

}}