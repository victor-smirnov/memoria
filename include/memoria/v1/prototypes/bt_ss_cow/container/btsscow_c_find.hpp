
// Copyright 2015 Victor Smirnov
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

#include <memoria/v1/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/v1/prototypes/bt/bt_macros.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <vector>

namespace memoria {
namespace v1 {

MEMORIA_V1_CONTAINER_PART_BEGIN(btss_cow::FindName)
public:
    using typename Base::Types;
    using typename Base::IteratorPtr;

protected:
    using typename Base::NodeBaseG;
    using typename Base::NodeDispatcher;
    using typename Base::LeafDispatcher;
    using typename Base::BranchDispatcher;
    using typename Base::Position;
    using typename Base::BranchNodeEntry;
    using typename Base::BlockUpdateMgr;
    using typename Base::CtrSizeT;

    using SplitFn = std::function<BranchNodeEntry (NodeBaseG&, NodeBaseG&)>;
    using MergeFn = std::function<void (const Position&)>;

    using Base::Streams;

public:
    auto size() const {
        return self().sizes()[0];
    }

    auto seek(CtrSizeT position)
    {
        return self().template seek_stream<0>(position);
    }

    auto Begin() {
        return self().seek(0);
    }

    auto End()
    {
        auto size = self().size();
        if (size > 0)
        {
            return self().seek(size);
        }
        else {
            return self().seek(0);
        }
    }

    auto begin() {
        return self().Begin();
    }

    auto end() {
        return self().End();
    }

    auto endm()
    {
        return IterEndMark();
    }

MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(btss_cow::LeafCommonName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS





#undef M_TYPE
#undef M_PARAMS

}}
