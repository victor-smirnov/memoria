
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
namespace multimap {

template <typename Types, typename Profile, typename IteratorPtr>
class KeysIteratorImpl: public IKeysScanner<Types, Profile> {
    using Base = IKeysScanner<Types, Profile>;

    using typename Base::KeyView;
    using typename Base::ValueView;
    using typename Base::KeysIOVSubstreamAdapter;

    using Base::keys_;

    int32_t idx_;

    IteratorPtr iter_;
public:
    KeysIteratorImpl(IteratorPtr iter):
        iter_(iter)
    {
        idx_ = iter_->iter_leafrank(iter_->iter_local_pos(), 0);
        build();
    }

    virtual bool is_end() const {
        return iter_->iter_is_end();
    }

    virtual void next()
    {
        size_t keys_size = keys_.array().size();
        iter_->iter_btfl_select_fw(keys_size, 0); // next leaf with keys;
        idx_ = 0;

        build();
    }

    virtual void dump_iterator() const
    {
        iter_->dump();
    }

    virtual CtrSharedPtr<IValuesScanner<Types, Profile>> values(size_t key_idx)
    {
        auto ii = iter_->iter_clone();

        ii->iter_btfl_select_fw(key_idx, 0);
        ii->to_values();

        auto ptr = ctr_make_shared<multimap::ValuesIteratorImpl<Types, Profile, IteratorPtr>>(ii);
        return memoria_static_pointer_cast<IValuesScanner<Types, Profile>>(ptr);
    }

private:

    void build()
    {
        const io::IOVector& buffer = iter_->iovector_view();
        int32_t iter_leaf_size = iter_->iter_leaf_size(0);

        keys_.clear();
        KeysIOVSubstreamAdapter::read_to(buffer.substream(0), 0, idx_, iter_leaf_size - idx_, keys_.array());
    }
};

}
}}
