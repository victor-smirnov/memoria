
// Copyright 2011-2022 Victor Smirnov
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

#include <memoria/core/types.hpp>

#pragma once

namespace memoria {

template <typename Types>
struct CtrTypesT: Types {
    using List = typename Types::CtrList;

    template <typename Types_>
    using BaseFactory = typename Types::template CtrBaseFactory<Types_>;
};


template <typename Types>
struct RWCtrTypesT: Types {
    using List = typename Types::RWCtrList;

    template <typename Types_>
    using BaseFactory = typename Types::template RWCtrBaseFactory<Types_>;

    using ROTypes = CtrTypesT<Types>;
};


template <typename Types>
struct BlockIterStateTypesT: Types {
    using Base = Types;
    using List = typename Types::BlockIterStateList;
    using IteratorInterface = EmptyType;

    template <typename Types_>
    using BaseFactory = typename Types::template BlockIterStateBaseFactory<Types_>;
};

}
