
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

#include <memoria/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/core/container/container.hpp>

#include <memoria/core/packed/sseq/packed_ssrle_seq.hpp>

namespace memoria {
namespace btfl {


template <int32_t DataStreams> struct StructureStreamTF: HasType<PkdSSRLESeqT<DataStreams>> {};





template <typename Iterator, typename Container>
class BTFLIteratorPrefixCache: public bt::BTreeIteratorPrefixCache<Iterator, Container> {

    using Base      = bt::BTreeIteratorPrefixCache<Iterator, Container>;
    using Position  = typename Container::Types::Position;

    static const int32_t Streams = Container::Types::Streams;



public:
    using MyType = BTFLIteratorPrefixCache<Iterator, Container>;

    BTFLIteratorPrefixCache()
    {}

    bool operator==(const MyType& other) const
    {
        return Base::operator==(other);
    }

    bool operator!=(const MyType& other) const
    {
        return Base::operator!=(other);
    }
};



template <
    typename I, typename C
>
std::ostream& operator<<(std::ostream& out, const BTFLIteratorPrefixCache<I, C>& iter_cache)
{
    out<<"BTFLIteratorPrefixCache[";
    out<<"Branch prefixes: "<<iter_cache.prefixes()<<", Leaf Prefixes: "<<iter_cache.leaf_prefixes();
    out<<", Size Prefixes: "<<iter_cache.size_prefix();
    out<<"]";

    return out;
}



}}
