
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

#include <memoria/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>
#include <memoria/core/container/macros.hpp>

#include <vector>

namespace memoria {

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
    using Base::Streams;

public:
    Result<ProfileCtrSizeT<Profile>> size() const noexcept
    {
        using ResultT = Result<ProfileCtrSizeT<Profile>>;
        MEMORIA_TRY(sizes, self().sizes());

        return ResultT::of(sizes[0]);
    }

    auto ctr_seek(CtrSizeT position) const noexcept
    {
        return self().template ctr_seek_stream<0>(position);
    }

    auto ctr_begin() const noexcept {
        return self().ctr_seek(0);
    }

    Result<IteratorPtr> ctr_end() const noexcept
    {
        MEMORIA_TRY(size, self().size());

        if (size > 0)
        {
            return self().ctr_seek(size);
        }
        else {
            return self().ctr_seek(0);
        }
    }



MEMORIA_V1_CONTAINER_PART_END


#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::LeafCommonName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS





#undef M_TYPE
#undef M_PARAMS

}
