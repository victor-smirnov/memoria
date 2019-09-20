
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

MEMORIA_V1_CONTAINER_PART_BEGIN(btss::FindName)
public:
    using typename Base::Types;
    using typename Base::IteratorPtr;

protected:
    using typename Base::NodeBaseG;
    using typename Base::Position;
    using typename Base::BranchNodeEntry;
    using typename Base::BlockUpdateMgr;
    using typename Base::CtrSizeT;

    using Profile = typename Types::Profile;

    using SplitFn = std::function<BranchNodeEntry (NodeBaseG&, NodeBaseG&)>;
    using MergeFn = std::function<void (const Position&)>;

    using Base::Streams;

public:
    ProfileCtrSizeT<Profile> size() const {
        return self().sizes()[0];
    }

    auto ctr_seek(CtrSizeT position)
    {
        return self().template ctr_seek_stream<0>(position);
    }

    auto ctr_begin() {
        return self().ctr_seek(0);
    }

    auto ctr_end()
    {
        auto size = self().size();
        if (size > 0)
        {
            return self().ctr_seek(size);
        }
        else {
            return self().ctr_seek(0);
        }
    }

    auto begin() {
        return self().ctr_begin();
    }

    auto end() {
        return self().ctr_end();
    }

    auto endm()
    {
        return IterEndMark();
    }

MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::LeafCommonName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS





#undef M_TYPE
#undef M_PARAMS

}}
