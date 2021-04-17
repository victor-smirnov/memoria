
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


#include <memoria/containers/vector/vctr_names.hpp>

#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>


namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(mvector::CtrApiVLenName)

public:
    using Types = typename Base::Types;


    using typename Base::TreeNodePtr;
    using typename Base::Position;
    using typename Base::BranchNodeEntry;
    using typename Base::BlockUpdateMgr;
    using typename Base::CtrSizeT;
    using typename Base::IteratorPtr;
    using typename Base::Profile;

protected:
    using Value = typename Types::Value;
    using ValueDataType = typename Types::ValueDataType;
    using ViewType  = DTTViewType<ValueDataType>;

public:

    using typename Base::BranchNodeExtData;
    using typename Base::LeafNodeExtData;
    using typename Base::ContainerTypeName;



MEMORIA_V1_CONTAINER_PART_END

}
