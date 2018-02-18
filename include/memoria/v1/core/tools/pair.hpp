
// Copyright 2016 Victor Smirnov
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

#include <iostream>
#include <type_traits>
#include <memory>

namespace memoria {
namespace v1 {

class ScriptPair {
    void* ptr_;
public:
    ScriptPair(void* ptr): ptr_(ptr) {}

    virtual ~ScriptPair() noexcept {}

    void* ptr() {return ptr_;}
    const void* ptr() const {return ptr_;}
};

using PairPtr = std::unique_ptr<ScriptPair>;

}}
