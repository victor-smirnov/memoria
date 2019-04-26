
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
#include <memoria/v1/core/tools/dump.hpp>

#include <memoria/v1/containers/vector/vctr_names.hpp>
#include <memoria/v1/containers/vector/vctr_tools.hpp>
#include <memoria/v1/core/container/iterator.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <memoria/v1/prototypes/bt/bt_macros.hpp>

#include <iostream>

namespace memoria {
namespace v1 {

MEMORIA_V1_ITERATOR_PART_BEGIN(mvector::ItrApiName)

    typedef Ctr<typename Types::CtrTypes>                                       Container;

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBaseG                                            NodeBaseG;

    typedef typename Container::Value                                           Value;
    typedef typename Container::BranchNodeEntry                                     BranchNodeEntry;

    typedef typename Container::Types::Blocks::LeafDispatcher                    LeafDispatcher;
    typedef typename Container::Position                                        Position;

    using CtrSizeT = typename Container::Types::CtrSizeT;

public:


    template <typename Iterator>
    class EntryAdaptor {
        Iterator current_;

        Value value_;

    public:
        EntryAdaptor(Iterator current): current_(current) {}

        template <typename V>
        void put(StreamTag<0>, StreamTag<0>, int32_t block, V&& entry) {}

        template <typename V>
        void put(StreamTag<0>, StreamTag<1>, int32_t block, V&& value) {
            value_ = value;
        }

        void next()
        {
            *current_ = value_;
            current_++;
        }
    };


    template <typename OutputIterator>
    auto read(OutputIterator iter, CtrSizeT length)
    {
        auto& self = this->self();

        EntryAdaptor<OutputIterator> adaptor(iter);

        return self.ctr().template read_entries<0>(self, length, adaptor);
    }

    template <typename OutputIterator>
    auto read(OutputIterator iter)
    {
        auto& self = this->self();
        return read(iter, self.ctr().size());
    }


    Value value() const
    {
        auto me = this->self();

        auto v = me.read((CtrSizeT)1);

        if (v.size() == 1)
        {
            return v[0];
        }
        else if (v.size() == 0)
        {
            MMA1_THROW(Exception()) << WhatCInfo("Attempt to read vector after its end");
        }
        else {
            MMA1_THROW(Exception()) << WhatCInfo("Invalid vector read");
        }
    }

    std::vector<Value> read(CtrSizeT size)
    {
        auto& self = this->self();

        auto pos = self.pos();
        auto ctr_size = self.ctr().size();

        auto length = pos + size <= ctr_size ? size : ctr_size - pos;

        std::vector<Value> data;

        self.read(std::back_inserter(data), length);

        return data;
    }

    struct ForEachFn {
        template <typename StreamObj, typename Fn>
        void stream(const StreamObj* obj, int32_t from, int32_t to, Fn&& fn)
        {
            obj->for_each(from, to, fn);
        }
    };

    template <typename Fn>
    CtrSizeT for_each(CtrSizeT length, Fn&& fn)
    {
        auto& self = this->self();

        return self.ctr().template read_substream<IntList<0, 1>>(self, 0, length, std::forward<Fn>(fn));
    }


    auto seek(CtrSizeT pos)
    {
        auto& self = this->self();

        CtrSizeT current_pos = self.pos();
        self.skip(pos - current_pos);
    }

MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(mvector::ItrApiName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS

#undef M_TYPE
#undef M_PARAMS

}}
