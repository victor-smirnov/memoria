
// Copyright 2019 Victor Smirnov
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

#include <memoria/v1/api/multimap/multimap_output.hpp>

namespace memoria {
namespace v1 {
namespace mmap {

template <typename Key, typename Value, typename IteratorPtr>
class KeysIteratorImpl: public IKeysIterator<Key, Value> {
    using Base = IKeysIterator<Key, Value>;

    using Base::keys_;
    using Base::size_;

    int32_t idx_;
    int32_t leaf_size_;

    IteratorPtr iter_;
public:
    KeysIteratorImpl(IteratorPtr iter):
        iter_(iter)
    {
        idx_ = iter_->leafrank(iter_->local_pos(), 0);
        build();
    }

    virtual bool is_end() const {
        return iter_->isEnd();
    }

    virtual void next()
    {
        iter_->selectFw(leaf_size_ - idx_, 0); // next leaf with keys;
        idx_ = iter_->leafrank(0, iter_->local_pos());

        build();
    }

    virtual void dump_iterator() const
    {
        iter_->dump();
    }

    virtual CtrSharedPtr<IValuesIterator<Value>> values(size_t key_idx)
    {
        auto ii = iter_->clone();

        ii->selectFw(key_idx, 0);
        ii->to_values();

        auto ptr = ctr_make_shared<mmap::ValuesIteratorImpl<Value, IteratorPtr>>(ii);
        return static_pointer_cast<IValuesIterator<Value>>(ptr);
    }

private:

    void build()
    {
        auto& buffer = iter_->iovector_view();
        auto& s0 = io::substream_cast<const io::IOColumnwiseFixedSizeArraySubstream<Key>>(buffer.substream(0));

        leaf_size_ = s0.describe(0).size;

        keys_ = s0.select(0, idx_);
        size_ = leaf_size_ - idx_;
    }
};

}
}}
