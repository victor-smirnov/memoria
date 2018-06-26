
// Copyright 2017 Victor Smirnov
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



#include <memoria/v1/containers/db/update_log/update_log_impl.hpp>
#include <memoria/v1/allocators/inmem/common/container_collection_cfg.hpp>

namespace memoria {
namespace v1 {

using Profile = DefaultProfile<>;    
using CtrName = UpdateLog;

template class update_log::SnapshotIDIterator<IterApi<UpdateLog, Profile>, int64_t>;
template class update_log::ContainerNameIterator<IterApi<UpdateLog, Profile>, int64_t>;
template class update_log::CommandsDataIterator<IterApi<UpdateLog, Profile>, int64_t>;

MMA1_INSTANTIATE_CTR_BTFL(CtrName, Profile)
    
}}

