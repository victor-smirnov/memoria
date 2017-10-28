
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

#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <memory>


namespace memoria {
namespace v1 {
namespace reactor {

#ifdef _WIN32    

namespace details {
	template<typename T>
	struct aligned_delete {
		void operator()(T* ptr) const {
			_aligned_free(ptr);
		}
	};
}

using DMABuffer = std::unique_ptr<uint8_t, details::aligned_delete<uint8_t>>;

DMABuffer allocate_dma_buffer(size_t size);
    
#else
    
namespace details {
	template<typename T>
	struct aligned_delete {
		void operator()(T* ptr) const {
			free(ptr);
		}
	};
}

using DMABuffer = std::unique_ptr<uint8_t, details::aligned_delete<uint8_t>>;

DMABuffer allocate_dma_buffer(size_t size);

#endif
    
}}}

