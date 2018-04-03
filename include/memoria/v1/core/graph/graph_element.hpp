
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
#include <memoria/v1/core/graph/graph_property.hpp>
#include <memoria/v1/core/graph/graph_collection.hpp>



namespace memoria {
namespace v1 {

struct IElement {
    virtual ~IElement() noexcept {}

    virtual Graph graph() const = 0;

    virtual Any id() const = 0;
    virtual U16String label() const = 0;

    virtual void remove() = 0;
    virtual bool is_removed() const = 0;
};



class Element: public PimplBase<IElement> {
    using Base = PimplBase<IElement>;
public:
    MMA1_PIMPL_DECLARE_DEFAULT_FUNCTIONS(Element)

    Graph graph() const;

    Any id() const {
        return this->ptr_->id();
    }

    U16String label() const {
        return this->ptr_->label();
    }

    void remove() {
        return this->ptr_->remove();
    }

    bool is_removed() const {
        return this->ptr_->is_removed();
    }
};



}}
