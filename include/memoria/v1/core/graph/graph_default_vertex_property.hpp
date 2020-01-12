
// Copyright 2018 Victor Smirnov
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


#include <memoria/v1/core/graph/graph_property.hpp>
#include <memoria/v1/core/graph/graph_collection.hpp>
#include <memoria/v1/core/memory/smart_ptrs.hpp>

#include <vector>

namespace memoria {
namespace v1 {



class DefaultVertexProperty: public IVertexProperty {
    Vertex vertex_;
    U8String key_;
    Any value_;
public:
    DefaultVertexProperty(Vertex vertex, const U8String& key, Any value):
        vertex_(vertex), key_(key), value_(value)
    {}

    virtual const U8String& key() const
    {
        return key_;
    }

    virtual const Any& value() const
    {
        return value_;
    }

    virtual bool is_empty() const
    {
        return false;
    }

    virtual void remove()
    {
        MMA1_THROW(GraphException()) << WhatCInfo("Can't remove property");
    }

    virtual bool is_removed() const
    {
        return false;
    }

    virtual Collection<Property> properties() const
    {
        return EmptyCollection<Property>::make();
    }

    static VertexProperty make(Vertex vertex, const U8String& key, const Any& value)
    {
        return VertexProperty(StaticPointerCast<IVertexProperty>(MakeLocalShared<DefaultVertexProperty>(vertex, key, value)));
    }
};





template <typename Fn>
class FnVertexProperty: public IVertexProperty {
    Vertex vertex_;
    U8String key_;
    Fn fn_;
    mutable Any value_;
public:
    FnVertexProperty(Vertex vertex, const U8String& key, Fn&& fn):
        vertex_(vertex), key_(key), fn_(std::move(fn))
    {}

    virtual const U8String& key() const
    {
        return key_;
    }

    virtual const Any& value() const
    {
        value_ = Any(fn_());
        return value_;
    }

    virtual bool is_empty() const
    {
        return false;
    }

    virtual void remove()
    {
        MMA1_THROW(GraphException()) << WhatCInfo("Can't remove property");
    }

    virtual bool is_removed() const
    {
        return false;
    }

    virtual Collection<Property> properties() const
    {
        return EmptyCollection<Property>::make();
    }
};




template <typename Fn>
static VertexProperty make_fn_vertex_property(Vertex vertex, const U8String& key, Fn&& fn)
{
    return VertexProperty(StaticPointerCast<IVertexProperty>(MakeLocalShared<FnVertexProperty<Fn>>(vertex, key, std::forward<Fn>(fn))));
}




namespace _ {

struct MakeFnVertexPropertiesHelper
{
    template <typename Container, typename Key, typename ValueFn, typename... Args>
    static void process(Container& ctr, const Vertex& vx, Key&& key, ValueFn&& fn, Args&&... args)
    {
        ctr.emplace_back(make_fn_vertex_property(
            vx,
            std::forward<Key>(key),
            std::forward<ValueFn>(fn)
        ));

        process(ctr, vx, std::forward<Args>(args)...);
    }

    template <typename Container>
    static void process(Container& ctr, const Vertex& vx) {}
};

struct MakeDefaultVertexPropertiesHelper
{
    template <typename Container, typename Key, typename ValueFn, typename... Args>
    static void process(Container& ctr, const Vertex& vx, Key&& key, ValueFn&& fn, Args&&... args)
    {
        ctr.emplace_back(DefaultVertexProperty::make(
            vx,
            std::forward<Key>(key),
            std::forward<ValueFn>(fn)
        ));

        process(ctr, vx, std::forward<Args>(args)...);
    }

    template <typename Container>
    static void process(Container& ctr, const Vertex& vx) {}
};


}





template <typename... Args>
Collection<VertexProperty> make_fn_vertex_properties(Vertex vertex, Args&&... args)
{
    static_assert(sizeof...(args) %2 == 0, "args list size must be even");

    std::vector<VertexProperty> properties;

    _::MakeFnVertexPropertiesHelper::process(properties, vertex, std::forward<Args>(args)...);

    return STLCollection<VertexProperty, std::vector<VertexProperty>>::make(std::move(properties));
}


template <typename... Args>
Collection<VertexProperty> make_default_vertex_properties(Vertex vertex, Args&&... args)
{
    static_assert(sizeof...(args) %2 == 0, "args list size must be even");

    std::vector<VertexProperty> properties;

    _::MakeDefaultVertexPropertiesHelper::process(properties, vertex, std::forward<Args>(args)...);

    return STLCollection<VertexProperty, std::vector<VertexProperty>>::make(std::move(properties));
}




}}
