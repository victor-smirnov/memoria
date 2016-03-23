
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

#include <memoria/v1/core/container/names.hpp>
#include <memoria/v1/core/container/pages.hpp>

#include <memoria/v1/core/container/allocator.hpp>

namespace memoria {
namespace v1 {


class AbstractTransaction {
public:
    AbstractTransaction() {}
};


template <
    typename Profile, typename IDValueType = BigInt, int FlagsCount = 32, typename TransactionType = AbstractTransaction
>
struct BasicContainerCollectionCfg {

    typedef UUID                                                                ID;
    typedef AbstractPage <ID, FlagsCount>                                       Page;
    typedef TransactionType                                                     Transaction;

    typedef IAllocator<Page>                                                    AbstractAllocator;

    typedef AbstractAllocator                                                   AllocatorType;
};


}}