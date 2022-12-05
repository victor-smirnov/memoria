
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


namespace memoria::hermes {

class TinyObjectBase {
    TinyObjectMapPtr ptr_;
public:
    TinyObjectBase() noexcept {};
    TinyObjectBase(const TinyObjectMapPtr& ptr) noexcept:
        ptr_(ptr)
    {};
};

class Record: public TinyObjectBase {
public:
    Record() noexcept {}
    Record(const TinyObjectMapPtr& ptr) noexcept:
        TinyObjectBase(ptr)
    {}
};

}
