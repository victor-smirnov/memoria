
// Copyright 2015 Victor Smirnov
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


#include <memoria/v1/containers/multimap/mmap_names.hpp>
#include <memoria/v1/containers/multimap/mmap_tools.hpp>
#include <memoria/v1/core/container/container.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <vector>

namespace memoria {
namespace v1 {

MEMORIA_V1_CONTAINER_PART_BEGIN(mmap::CtrApiName)
public:
    using Types = typename Base::Types;
    using typename Base::IteratorPtr;

protected:
    using typename Base::NodeBaseG;
    using typename Base::NodeDispatcher;
    using typename Base::LeafDispatcher;
    using typename Base::BranchDispatcher;
    using typename Base::Position;
    using typename Base::BranchNodeEntry;
    using typename Base::BlockUpdateMgr;
    using typename Base::CtrSizeT;
    using typename Base::CtrSizesT;

    using Key   = typename Types::Key;
    using Value = typename Types::Value;

    using IOBuffer = DefaultIOBuffer;

public:
    IteratorPtr begin() {
        return self().template seek_stream<0>(0);
    }

    IteratorPtr end() {
        auto& self = this->self();
        return self.template seek_stream<0>(self.sizes()[0]);
    }

    //using Base::seek;

    IteratorPtr seek(CtrSizeT idx)
    {
        auto& self = this->self();
        auto ii = self.template seek_stream<0>(idx);

        ii->stream() = 0;

        ii->toStructureStream();

        return ii;
    }

    CtrSizeT size() const {
        return self().sizes()[0];
    }

    IteratorPtr find(Key key)
    {
        return self().template find_max_ge<IntList<0, 1>>(0, key);
    }

    IteratorPtr find_or_create(Key key)
    {
        auto& self = this->self();

        auto iter = self.find(key);

        if (!iter->is_found(key))
        {
            iter->insert_key(key);
        }

        return iter;
    }

    template <typename Iterator>
    IteratorPtr find_or_create(const Key& key, const Iterator& start, const Iterator& end)
    {
        auto& self = this->self();

        auto iter = self.find(key);

        if (!iter->is_found(key))
        {
            mmap::MultimapEntryBufferProducer<IOBuffer, Key, Iterator> producer(key, start, end);

            iter->bulkio_insert(producer);
        }

        return iter;
    }

protected:

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(mmap::CtrApiName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}}
