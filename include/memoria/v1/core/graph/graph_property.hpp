
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

#include <memoria/v1/core/strings/string.hpp>
#include <memoria/v1/core/strings/format.hpp>
#include <memoria/v1/core/memory/smart_ptrs.hpp>
#include <memoria/v1/core/exceptions/exceptions.hpp>

#include <memoria/v1/core/tools/pimpl_base.hpp>
#include <memoria/v1/core/tools/ptr_cast.hpp>

#include <memoria/v1/core/graph/graph_commons.hpp>
#include <memoria/v1/core/graph/graph_collection.hpp>





namespace memoria {
namespace v1 {

struct IProperty {
    virtual ~IProperty() noexcept {}

    virtual const U8String& key() const = 0;
    virtual const Any& value() const = 0;

    virtual bool is_empty() const = 0;

    virtual void remove() = 0;
    virtual bool is_removed() const = 0;
};

struct IVertexProperty: IProperty {
    virtual Collection<Property> properties() const = 0;
};






class Property: public PimplBase<IProperty> {
    using Base = PimplBase<IProperty>;
public:
    MMA1_PIMPL_DECLARE_DEFAULT_FUNCTIONS(Property)

    const U8String& key() const {
        return this->ptr_->key();
    }

    const Any& value() const {
        return this->ptr_->value();
    }

    bool is_empty() const {
        return this->ptr_->is_empty();
    }

    void remove() {
        return this->ptr_->remove();
    }

    bool is_removed() const {
        return this->ptr_->is_removed();
    }
};


class VertexProperty: public PimplBase<IVertexProperty> {
    using Base = PimplBase<IVertexProperty>;
public:
    MMA1_PIMPL_DECLARE_DEFAULT_FUNCTIONS(VertexProperty)


    const U8String& key() const {
        return this->ptr_->key();
    }

    const Any& value() const {
        return this->ptr_->value();
    }

    bool is_empty() const {
        return this->ptr_->is_empty();
    }

    void remove() {
        return this->ptr_->remove();
    }

    bool is_removed() const {
        return this->ptr_->is_removed();
    }

    Collection<Property> properties() const {
        return this->ptr_->properties();
    }

    Property as_property() {
        return Property(StaticPointerCast<IProperty>(this->ptr_));
    }
};


struct EmptyProperty: IProperty {
    virtual const U8String& key() const {
        MMA1_THROW(GraphException()) << WhatCInfo("No such property");
    }

    virtual const Any& value() const {
        MMA1_THROW(GraphException()) << WhatCInfo("No such property");
    }

    virtual void remove() {
        MMA1_THROW(GraphException()) << WhatCInfo("No such property");
    }

    virtual bool is_removed() const {
        MMA1_THROW(GraphException()) << WhatCInfo("No such property");
    }

    virtual bool is_empty() const {
        return true;
    }
};




struct EmptyVertexProperty: IVertexProperty {

    virtual const U8String& key() const {
        MMA1_THROW(GraphException()) << WhatCInfo("No such property");
    }

    virtual Any& value() const {
        MMA1_THROW(GraphException()) << WhatCInfo("No such property");
    }

    virtual void remove() {
        MMA1_THROW(GraphException()) << WhatCInfo("No such property");
    }

    virtual bool is_removed() const {
        MMA1_THROW(GraphException()) << WhatCInfo("No such property");
    }

    virtual bool is_empty() const {
        return true;
    }

    virtual Collection<Property> properties() const {
        MMA1_THROW(GraphException()) << WhatCInfo("No such property");
    }
};

Property empty_property();
VertexProperty empty_vertex_property();


}}
