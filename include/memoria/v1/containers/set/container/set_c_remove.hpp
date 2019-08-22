
// Copyright 2014 Victor Smirnov
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

#include <memoria/v1/containers/map/map_names.hpp>
#include <memoria/v1/containers/map/map_tools.hpp>
#include <memoria/v1/core/container/container.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <vector>

namespace memoria {
namespace v1 {

MEMORIA_V1_CONTAINER_PART_BEGIN(set::CtrRemoveName)

public:
    using Types = typename Base::Types;

protected:
    using typename Base::NodeBaseG;

    typedef typename Types::Key                                                 Key;
    using KeyView   = typename DataTypeTraits<Key>::ViewType;
    using KeyV      = typename DataTypeTraits<Key>::ValueType;

    using Profile   = typename Types::Profile;

    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;

    typedef typename Types::BlockUpdateMgr                                      BlockUpdateMgr;
    
public:    
    bool remove(const KeyView& key)
    {
        auto iter = self().find(key);
        if ((!iter->isEnd()) && iter->key() == key) 
        {
            iter->remove();
            return true;
        }
        
        return false;
    }

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(set::CtrRemoveName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}}
