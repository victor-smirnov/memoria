
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

#include <memoria/v1/core/types.hpp>
#include <memoria/v1/core/tools/uuid.hpp>
#include <memoria/v1/core/container/container.hpp>

#include <memoria/v1/api/common/ctr_api_btss.hpp>
#include "ctr_impl.hpp"

#include <memory>

namespace memoria {
namespace v1 {

/*
template <typename CtrName, typename Profile>
int64_t CtrApiBTSSBase<CtrName, Profile>::size()
{
    return this->pimpl_->size();
}

template <typename CtrName, typename Profile>
typename CtrApiBTSSBase<CtrName, Profile>::Iterator CtrApiBTSSBase<CtrName, Profile>::begin() 
{
    return this->pimpl_->begin();
}


template <typename CtrName, typename Profile>
typename CtrApiBTSSBase<CtrName, Profile>::Iterator CtrApiBTSSBase<CtrName, Profile>::end() 
{
    return this->pimpl_->end();
}

template <typename CtrName, typename Profile>
typename CtrApiBTSSBase<CtrName, Profile>::Iterator CtrApiBTSSBase<CtrName, Profile>::seek(int64_t pos)
{
    return this->pimpl_->seek(pos);
}




template <typename CtrName, typename Profile>
bool IterApiBTSSBase<CtrName, Profile>::is_end() const
{
    return this->pimpl_->isEnd();
}


template <typename CtrName, typename Profile>
bool IterApiBTSSBase<CtrName, Profile>::next()
{
    return this->pimpl_->next();
}

template <typename CtrName, typename Profile>
bool IterApiBTSSBase<CtrName, Profile>::prev()
{
    return this->pimpl_->prev();
}


template <typename CtrName, typename Profile>
void IterApiBTSSBase<CtrName, Profile>::remove()
{
    return this->pimpl_->remove();
}

template <typename CtrName, typename Profile>
int64_t IterApiBTSSBase<CtrName, Profile>::remove(int64_t length)
{
    return this->pimpl_->remove(length);
}




template <typename CtrName, typename Profile>
int64_t IterApiBTSSBase<CtrName, Profile>::read_to(
        io::IOVectorConsumer& producer,
        int64_t length
)
{
    return this->pimpl_->read_to(producer, length);
}


template <typename CtrName, typename Profile>
int64_t IterApiBTSSBase<CtrName, Profile>::populate(
        io::IOVector& io_vector,
        int64_t length
)
{
    return this->pimpl_->populate(io_vector, length);
}


template <typename CtrName, typename Profile>
int64_t IterApiBTSSBase<CtrName, Profile>::insert(
        io::IOVectorProducer& producer,
        int64_t start,
        int64_t length
)
{
    return this->pimpl_->insert_iovector(producer, start, length);
}


template <typename CtrName, typename Profile>
int64_t IterApiBTSSBase<CtrName, Profile>::insert(
        io::IOVector& io_vector,
        int64_t start,
        int64_t length
)
{
    return this->pimpl_->insert_iovector(io_vector, start, length);
}


template <typename CtrName, typename Profile>
int64_t IterApiBTSSBase<CtrName, Profile>::pos()
{
    return this->pimpl_->pos();
}

template <typename CtrName, typename Profile>
int64_t IterApiBTSSBase<CtrName, Profile>::skip(int64_t offset)
{
    return this->pimpl_->skip(offset);
}
*/
}
}
