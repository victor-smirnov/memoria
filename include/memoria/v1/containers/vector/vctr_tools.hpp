
// Copyright 2013 Victor Smirnov
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

#include <memoria/v1/prototypes/bt/tools/bt_tools.hpp>

#include <memoria/v1/core/tools/static_array.hpp>
#include <memoria/v1/core/tools/bitmap.hpp>
#include <memoria/v1/core/container/container.hpp>

#include <memoria/v1/core/packed/array/packed_fse_array.hpp>
#include <memoria/v1/core/packed/array/packed_vle_dense_array.hpp>

#include <memoria/v1/prototypes/bt_ss/btss_input.hpp>

namespace memoria {
namespace v1 {
namespace mvector       {

template <typename KeyType, Int Selector> struct VectorValueStructTF;

template <typename KeyType>
struct VectorValueStructTF<KeyType, 1>: HasType<PkdFSQArrayT<KeyType>> {};

template <typename KeyType>
struct VectorValueStructTF<KeyType, 0>: HasType<PkdVDArrayT<KeyType>> {};

//
//template <typename CtrT, typename InputIterator, Int EntryBufferSize = 1000>
//class VectorIteratorInputProvider: public v1::btss::AbstractIteratorBTSSInputProvider<
//    CtrT,
//    VectorIteratorInputProvider<CtrT, InputIterator, EntryBufferSize>,
//    InputIterator
//>
//{
//    using Base = v1::btss::AbstractIteratorBTSSInputProvider<
//            CtrT,
//            VectorIteratorInputProvider<CtrT, InputIterator, EntryBufferSize>,
//            InputIterator
//    >;
//
//public:
//
//    using typename Base::CtrSizeT;
//private:
//    CtrSizeT one_ = 1;
//public:
//    VectorIteratorInputProvider(CtrT& ctr, const InputIterator& start, const InputIterator& end, Int capacity = 10000):
//        Base(ctr, start, end, capacity)
//    {}
//
//    const auto& buffer(StreamTag<0>, StreamTag<0>, Int idx, Int block) {
//        return one_;
//    }
//
//    const auto& buffer(StreamTag<0>, StreamTag<1>, Int idx, Int block) {
//        return Base::input_value_buffer_[idx];
//    }
//};
//




}
}}
