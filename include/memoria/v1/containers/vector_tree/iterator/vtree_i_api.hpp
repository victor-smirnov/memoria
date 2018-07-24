
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

#include <memoria/v1/core/container/container.hpp>

#include <memoria/v1/containers/vector_tree/vtree_names.hpp>
#include <memoria/v1/containers/vector/vctr_tools.hpp>

#include <functional>

namespace memoria {
namespace v1 {


MEMORIA_V1_ITERATOR_PART_BEGIN(vtree::ItrApiName)
public:
    typedef Ctr<VTreeCtrTypes<Types>>                           ContainerType;

    typedef typename ContainerType::Tree::Iterator              TreeIterator;
    typedef typename ContainerType::Vec::Iterator               VectorIterator;
    typedef typename ContainerType::Vec::Value                  Value;

    typedef typename ContainerType::Tree::Types::CtrSizeT       CtrSizeT;

    CtrSizeT next_siblings() const
    {
        return self().tree_iter().next_siblings();
    }

    CtrSizeT prev_siblings() const
    {
        return self().tree_iter().prev_siblings();
    }

    bool next_sibling()
    {
        auto& self = this->self();

        if (self.tree_iter().next_sibling())
        {
            auto data_base = self.tree_iter().template sumLabel<1>();
            self.vector_iter() = *self.ctr().vector()->seek(data_base).get();

            return true;
        }
        else {
            return false;
        }
    }

    bool prev_sibling()
    {
        auto& self = this->self();

        if (self.tree_iter().prev_sibling())
        {
            auto data_base = self.tree_iter().template sumLabel<1>();
            self.vector_iter() = *self.ctr().vector()->seek(data_base).get();

            return true;
        }
        else {
            return false;
        }
    }

    LoudsNode node() const
    {
        return self().tree_iter().node();
    }

    Value value() const
    {
        return self().vector_iter().value();
    }

    void insert(Value value)
    {
        self().vector_iter().insert(value);
        self().tree_iter().template addLabel<1>(1);
    }

    void insert(const std::vector<Value>& values)
    {
        auto& self = this->self();

        self.vector_iter().bulk_insert(values.begin(), values.end());

        self.tree_iter().template addLabel<1>(values.size());
    }

    void insert(std::vector<Value>& values)
    {
        self().vector_iter().insert(values);
        self().tree_iter().template addLabel<1>(values.size());
    }

    CtrSizeT lobBase() const
    {
        return self().tree_iter().template sumLabel<1>();
    }

    CtrSizeT lobSize() const
    {
        return std::get<1>(self().tree_iter().labels());
    }

    void removeLob()
    {
        auto& self = this->self();

        CtrSizeT data_base    = self.lobBase();
        CtrSizeT data_size    = self.lobSize();

        self.vector_iter().seek(data_base);
        self.vector_iter().remove(data_size);
    }

    void removeLob(CtrSizeT size)
    {
        auto& self = this->self();

        CtrSizeT data_base    = self.lobBase();
        CtrSizeT data_size    = self.lobSize();
        CtrSizeT data_pos     = self.vector_iter().local_pos();

        if (data_pos + size < data_base + data_size)
        {
            return self.vector_iter().remove(size);
        }
        else {
            return self.vector_iter().remove(data_base + data_size - data_pos);
        }
    }

    CtrSizeT skipFw(CtrSizeT delta)
    {
        auto& self = this->self();

        CtrSizeT data_base    = self.lobBase();
        CtrSizeT data_size    = self.lobSize();
        CtrSizeT data_pos     = self.vector_iter().local_pos();

        if (data_pos + delta < data_base + data_size)
        {
            return self.vector_iter().skipFw(delta);
        }
        else {
            return self.vector_iter().skipFw(data_base + data_size - data_pos);
        }
    }

    std::vector<Value> read()
    {
        CtrSizeT data_size = self().lobSize();
        auto& viter = self().vector_iter();
        return viter.read(data_size);
    }

MEMORIA_V1_ITERATOR_PART_END

}}
