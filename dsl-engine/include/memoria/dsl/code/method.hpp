
// Copyright 2023 Victor Smirnov
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


#include <memoria/core/hermes/schema.hpp>

#include <memoria/core/hermes/array/array_of.hpp>


namespace memoria::dsl {

struct Argument: public hermes::TinyObjectBase {
    static constexpr NamedCode NAME         = NamedCode(1, "name");
    static constexpr NamedCode METADATA     = NamedCode(2, "metadata");
    static constexpr NamedCode TYPE         = NamedCode(3, "type");
public:
    Argument(hermes::TinyObjectMap object):
        hermes::TinyObjectBase(std::move(object))
    {}

    Argument(hermes::Object object):
        hermes::TinyObjectBase(object.as_tiny_object_map())
    {}

    Argument(hermes::HermesCtr&& ctr):
        hermes::TinyObjectBase(ctr.make_tiny_map())
    {
        ctr.set_root(object_);
    }
};


struct Method: public hermes::TinyObjectBase {
    static constexpr NamedCode NAME         = NamedCode(1, "name");
    static constexpr NamedCode METADATA     = NamedCode(2, "metadata");
    static constexpr NamedCode ARGUMENTS    = NamedCode(3, "arguments");
    static constexpr NamedCode CONSTANTS    = NamedCode(4, "constants");
    static constexpr NamedCode RETURN_TYPE  = NamedCode(5, "return_type");
    static constexpr NamedCode CODE         = NamedCode(6, "code");

public:
    Method(hermes::TinyObjectMap object):
        hermes::TinyObjectBase(std::move(object))
    {}

    Method(hermes::HermesCtr&& ctr):
        hermes::TinyObjectBase(ctr.make_tiny_map())
    {
        ctr.set_root(object_);
    }

    U8StringOView name() const {
        return this->object_.expect(NAME).as_varchar();
    }

    hermes::ArrayOf<Argument> arguments() const {
        return hermes::ArrayOf<Argument>{this->object_.expect(ARGUMENTS).as_object_array()};
    }
};




}
