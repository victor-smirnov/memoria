
// Copyright 2011 Victor Smirnov
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

#include <memoria/v1/prototypes/bt/bt_names.hpp>

#include <memoria/v1/prototypes/composite/comp_names.hpp>
#include <memoria/v1/core/container/container.hpp>

namespace memoria {
namespace v1 {



template <typename Profile_, typename ContainerTypeSelector>
struct CompositeTypes {

    typedef Profile_                                                            Profile;

    typedef TypeList<
        v1::bt::AllocatorName
    >                                                                           ContainerPartsList;

    typedef TypeList<>                                                          IteratorPartsList;

    typedef EmptyType                                                           IteratorInterface;

    template <typename Types_>
    struct IterBaseFactory {
        typedef IteratorBase<Types_>                        Type;
    };

    template <typename Types_>
    struct CtrBaseFactory {
        typedef CtrBase<Types_>                             Type;
    };
};



template <typename Profile_, typename T>
class CtrTF<Profile_, v1::Composite, T> {

    using MyType = CtrTF<Profile_, Composite, T>;

    using Allocator = typename ContainerCollectionCfg<Profile_>::Types::AbstractAllocator;

public:

    struct Types {
        typedef Profile_                                    Profile;
		using Allocator = typename MyType::Allocator;

        typedef CtrTypesT<Types>                            CtrTypes;
        typedef IterTypesT<Types>                           IterTypes;
    };

    typedef typename Types::CtrTypes                                            CtrTypes;
    typedef typename Types::IterTypes                                           IterTypes;

    typedef Ctr<CtrTypes>                                                       Type;


};


}}