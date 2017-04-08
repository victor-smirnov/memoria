
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

#include <memory>

namespace memoria {
namespace v1 {


class AbstractTransaction {
public:
    AbstractTransaction() {}
};


template <typename T>
struct StdMakeSharedPtr {
	template <typename... Args>
	static auto make_shared(Args&&... args) {
		return std::make_shared<T>(std::forward<Args>(args)...);
	}
};

template <typename T>
struct StdMakeUniquePtr {
	template <typename... Args>
	static auto make_unique(Args&&... args) {
		return std::make_unique<T>(std::forward<Args>(args)...);
	}
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

    template <typename T>
    using CtrSharedPtr = std::shared_ptr<T>;
    
    template <typename T>
    using CtrUniquePtr = std::unique_ptr<T>;

    template <typename T>
    using CtrEnableSharedFromThis = std::enable_shared_from_this<T>;

    template <typename T>
    using CtrMakeSharedPtr = StdMakeSharedPtr<T>;
    
    template <typename T>
    using CtrMakeUniquePtr = StdMakeUniquePtr<T>;
};


template <typename Profile> class ContainerCollectionCfg;

template <typename Profile, typename T>
using CtrSharedPtr = typename ContainerCollectionCfg<Profile>::Types::template CtrSharedPtr<T>;

template <typename Profile, typename T>
using CtrUniquePtr = typename ContainerCollectionCfg<Profile>::Types::template CtrUniquePtr<T>;



template <typename Profile, typename T>
using CtrEnableSharedFromThis = typename ContainerCollectionCfg<Profile>::Types::template CtrEnableSharedFromThis<T>;

template <typename Profile, typename T>
using CtrMakeSharedPtr = typename ContainerCollectionCfg<Profile>::Types::template CtrMakeSharedPtr<T>;

template <typename Profile, typename T>
using CtrMakeUniquePtr = typename ContainerCollectionCfg<Profile>::Types::template CtrMakeUniquePtr<T>;


}}
