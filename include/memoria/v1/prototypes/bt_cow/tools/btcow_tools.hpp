
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

#include <memoria/v1/prototypes/bt/tools/bt_tools.hpp>

#pragma once
namespace memoria {
namespace v1 {
namespace btcow {







template <typename Iterator, typename Container>
class BTreeIteratorPrefixCache {

    using IteratorPrefix = typename Container::Types::IteratorBranchNodeEntry;
    using SizePrefix = core::StaticVector<int64_t, Container::Types::Streams>;

    using MyType = BTreeIteratorPrefixCache<Iterator, Container>;

    IteratorPrefix  prefix_;
    IteratorPrefix  leaf_prefix_;
    SizePrefix      size_prefix_;

public:

    void reset() {
        prefix_ = IteratorPrefix();
        leaf_prefix_ = IteratorPrefix();
        size_prefix_ = SizePrefix();
    }

    const SizePrefix& size_prefix() const
    {
        return size_prefix_;
    }

    SizePrefix& size_prefix()
    {
        return size_prefix_;
    }

    const IteratorPrefix& prefixes() const
    {
        return prefix_;
    }

    IteratorPrefix& prefixes()
    {
        return prefix_;
    }

    const IteratorPrefix& leaf_prefixes() const
    {
        return leaf_prefix_;
    }

    IteratorPrefix& leaf_prefixes()
    {
        return leaf_prefix_;
    }

    void initState() {}

    bool operator==(const MyType& other) const
    {
        return prefix_ == other.prefix_ && leaf_prefix_ == other.leaf_prefix_ && size_prefix_ == other.size_prefix_;
    }

    bool operator!=(const MyType& other) const
    {
        return prefix_ != other.prefix_ || leaf_prefix_ != other.leaf_prefix_ || size_prefix_ != other.size_prefix_;
    }
};

template <
    typename I, typename C
>
std::ostream& operator<<(std::ostream& out, const BTreeIteratorPrefixCache<I, C>& cache)
{
    out<<"IteratorPrefixCache[";
    out<<"Branch prefixes: "<<cache.prefixes()<<", Leaf Prefixes: "<<cache.leaf_prefixes()<<", Size Prefixes: "<<cache.size_prefix();
    out<<"]";

    return out;
}





}
}}
