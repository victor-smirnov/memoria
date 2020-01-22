
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

#include <memoria/core/strings/string.hpp>
#include <memoria/core/strings/format.hpp>
#include <memoria/core/memory/smart_ptrs.hpp>
#include <memoria/core/exceptions/exceptions.hpp>

#include <memoria/core/tools/pimpl_base.hpp>
#include <memoria/core/memory/ptr_cast.hpp>

#include <memoria/core/tools/any.hpp>


namespace memoria {

using GraphSizeT = uint64_t;

enum class Direction {EDGE_IN, EDGE_OUT, BOTH};

using LabelList = std::vector<U8String>;
using IDList = std::vector<Any>;

struct GraphException: virtual RuntimeException {};

class Graph;
class Vertex;
class Edge;
class Property;
class VertexProperty;

template <typename T>
class Collection;

template <typename T>
class Iterator;


static inline bool is_out(Direction dir) {
    return dir == Direction::BOTH || dir == Direction::EDGE_OUT;
}

static inline bool is_in(Direction dir) {
    return dir == Direction::BOTH || dir == Direction::EDGE_IN;
}

bool contains(const LabelList& list, const U8String& label);

}
