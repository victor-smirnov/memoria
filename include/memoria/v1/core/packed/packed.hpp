
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

// Packed structures currently in use. More to follow...

#include <memoria/v1/core/packed/datatypes/accessors_common.hpp>
#include <memoria/v1/core/packed/datatypes/fixed_size.hpp>
#include <memoria/v1/core/packed/datatypes/varchar.hpp>

#include <memoria/v1/core/packed/array/packed_fse_array.hpp>
#include <memoria/v1/core/packed/array/packed_vle_array.hpp>
#include <memoria/v1/core/packed/array/packed_fse_opt_array.hpp>
#include <memoria/v1/core/packed/array/packed_vle_opt_array.hpp>

#include <memoria/v1/core/packed/datatype_buffer/packed_datatype_buffer.hpp>

#include <memoria/v1/core/packed/misc/packed_empty_struct.hpp>
#include <memoria/v1/core/packed/misc/packed_sized_struct.hpp>
#include <memoria/v1/core/packed/misc/packed_map.hpp>
#include <memoria/v1/core/packed/misc/packed_tuple.hpp>

#include <memoria/v1/core/packed/sseq/packed_fse_searchable_seq.hpp>
#include <memoria/v1/core/packed/sseq/packed_rle_searchable_seq.hpp>

#include <memoria/v1/core/packed/tree/fse_max/packed_fse_optmax_tree.hpp>
#include <memoria/v1/core/packed/tree/vle_big/packed_vle_bigmax_tree.hpp>
