
// Copyright 2022 Victor Smirnov
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

#include <memoria/core/hermes/common.hpp>
#include <memoria/core/hermes/object.hpp>
#include <memoria/core/hermes/map/typed_map.hpp>
#include <memoria/core/hermes/container.hpp>
#include <memoria/core/hermes/array/object_array.hpp>


namespace memoria::hermes {

class TinyObjectBase {
protected:
    TinyObjectMap object_;
public:
    TinyObjectBase() noexcept {};

    TinyObjectBase(TinyObjectMap&& object) noexcept:
        object_(std::move(object))
    {};

    TinyObjectBase(HermesCtr& ctr) noexcept:
        object_(ctr.make_tiny_map())
    {
        ctr.set_root(object_);
    };

    TinyObjectBase(HermesCtr&& ctr) noexcept:
        object_(ctr.make_tiny_map())
    {
        ctr.set_root(object_);
    };


    bool is_null() const {
        return object_.is_null();
    }

    bool is_not_null() const {
        return object_.is_not_null();
    }

    const hermes::TinyObjectMap& object() const {return object_;}
    hermes::TinyObjectMap& object() {return object_;}

    U8String to_pretty_string() const {
        return object_.as_object().to_pretty_string();
    }

    U8String to_string() const {
        return object_.as_object().to_string();
    }
};

class Record: public TinyObjectBase {
public:
    Record() noexcept {}
    Record(TinyObjectMap ptr) noexcept:
        TinyObjectBase(std::move(ptr))
    {}
};




}
