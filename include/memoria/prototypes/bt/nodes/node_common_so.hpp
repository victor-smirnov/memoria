
// Copyright 2019 Victor Smirnov
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

#include <memoria/core/types.hpp>

#include <memoria/core/packed/tools/packed_stateful_dispatcher.hpp>
#include <memoria/core/packed/misc/packed_tuple.hpp>
#include <memoria/core/packed/misc/packed_map.hpp>


namespace memoria {


template <typename CtrT, typename NodeType_>
class NodeCommonSO {
protected:
    CtrT* ctr_;
    NodeType_* node_;
public:
//    static constexpr int32_t Streams = NodeType_::Streams;

    using NodeType = NodeType_;


    NodeCommonSO(): ctr_(), node_() {}
    NodeCommonSO(CtrT* ctr, NodeType* node):
        ctr_(ctr), node_(node)
    {}

    CtrT* ctr() const {return ctr_;}

    NodeType* node() {return node_;}
    const NodeType* node() const {return node_;}

};

}
