
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



#include <memoria/v1/api/set/set_api.hpp>
#include <memoria/v1/core/container/ctr_impl_btss.hpp>

#include <memory>

namespace memoria {
namespace v1 {


template <typename Key, typename IteratorPtr>
class SetIteratorImpl: public SetIterator<Key> {
    using KeyV   = typename DataTypeTraits<Key>::ValueType;
    IteratorPtr iter_;

public:
    SetIteratorImpl(IteratorPtr iter):
        iter_(iter)
    {}

    virtual KeyV key() const
    {
        return iter_->key();
    }

    virtual bool is_end() const
    {
        return iter_->iter_is_end();
    }

    virtual void next() {
        iter_->next();
    }
};
 

template <typename Key, typename Profile>
void ICtrApi<Set<Key>, Profile>::init_profile_metadata()
{
    SharedCtr<Set<Key>, ProfileAllocatorType<Profile>, Profile>::init_profile_metadata();
}

    
}}
