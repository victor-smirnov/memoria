
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

#include <memoria/core/types.hpp>
#include <memoria/core/types/list/append.hpp>

#include <memoria/core/types/mp11.hpp>

namespace memoria {


template <typename List, int32_t Len>
using SublistFromStart = boost::mp11::mp_take_c<List, Len>;

template <typename List, int32_t From>
using SublistToEnd = boost::mp11::mp_drop_c<List, From>;

namespace _ {
    template <typename List, int32_t From, int32_t To>
    struct SublistH {
        static_assert(From <= To, "Form must be <= To");

        using Type = SublistFromStart<
            SublistToEnd<List, From>,
            To - From
        >;
    };
}

template <typename List, int32_t From, int32_t To>
using Sublist = typename _::SublistH<List, From, To>::Type;

}
