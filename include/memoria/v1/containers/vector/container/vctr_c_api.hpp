
// Copyright 2013 Victor Smirnov
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


#include <memoria/v1/containers/vector/vctr_names.hpp>

#include <memoria/v1/core/container/container.hpp>
#include <memoria/v1/core/container/macros.hpp>



namespace memoria {
namespace v1 {

MEMORIA_V1_CONTAINER_PART_BEGIN(mvector::CtrApiName)

public:
    using Types = typename Base::Types;

protected:
    using typename Base::NodeBaseG;
    using typename Base::Position;
    using typename Base::BranchNodeEntry;
    using typename Base::BlockUpdateMgr;
    using typename Base::CtrSizeT;

    using Value = typename Types::Value;

public:
    CtrSizeT size() const {
        return self().sizes()[0];
    }

    auto seek(CtrSizeT pos)
    {
        typename Types::template SkipForwardWalker<Types, IntList<0>> walker(pos);

        return self().find_(walker);
    }


    MyType& operator<<(const std::vector<Value>& v)
    {
        auto& self = this->self();
        auto i = self.seek(self.size());
        i.insert_v(v);
        return self;
    }

MEMORIA_V1_CONTAINER_PART_END

}}
