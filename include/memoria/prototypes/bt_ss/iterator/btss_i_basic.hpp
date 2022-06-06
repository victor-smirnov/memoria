
// Copyright 2022 Victor Smirnov
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

#include <memoria/core/container/macros.hpp>
#include <memoria/core/types.hpp>
#include <memoria/prototypes/bt_ss/btss_names.hpp>

#include <memoria/core/iovector/io_vector.hpp>

#include <memoria/prototypes/bt_ss/btss_input_iovector.hpp>

#include <memoria/core/packed/tools/packed_allocator_types.hpp>

namespace memoria {

using bt::StreamTag;

MEMORIA_V1_ITERATOR_PART_BEGIN(btss::IteratorBasicName)

protected:
  using typename Base::CtrSizeT;
  using typename Base::TreePathT;
  using typename Base::Container;
  using typename Base::Position;

public:
    //FIXME: make protected again after AllocationMap refactoring
  CtrSizeT leaf_position_;

public:

  virtual void iter_reset_caches() = 0;

  const CtrSizeT& iter_leaf_position() const {
    return leaf_position_;
  }

  void iter_set_leaf_position(const CtrSizeT& pos) {
    leaf_position_ = pos;
  }

MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(btss::IteratorBasicName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS




#undef M_PARAMS
#undef M_TYPE

}
