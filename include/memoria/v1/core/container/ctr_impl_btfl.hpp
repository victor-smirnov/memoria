
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

#include <memoria/v1/api/common/ctr_api_btfl.hpp>
#include "ctr_impl.hpp"

#include <memory>

namespace memoria {
namespace v1 {



template <typename CtrName, typename Profile>
int64_t CtrApiBTFLBase<CtrName, Profile>::seq_size() const
{
    return this->pimpl_->seq_size();
}

template <typename CtrName, typename Profile>
typename CtrApiBTFLBase<CtrName, Profile>::Iterator CtrApiBTFLBase<CtrName, Profile>::seq_begin() 
{
    return this->pimpl_->seq_begin();
}


template <typename CtrName, typename Profile>
typename CtrApiBTFLBase<CtrName, Profile>::Iterator CtrApiBTFLBase<CtrName, Profile>::seq_end() 
{
    return this->pimpl_->seq_end();
}

template <typename CtrName, typename Profile>
typename CtrApiBTFLBase<CtrName, Profile>::Iterator CtrApiBTFLBase<CtrName, Profile>::seq_seek(int64_t pos)
{
    return this->pimpl_->seq_seek(pos);
}

template <typename CtrName, typename Profile>
typename CtrApiBTFLBase<CtrName, Profile>::Iterator CtrApiBTFLBase<CtrName, Profile>::seq_select(int64_t rank, int32_t sym)
{
    return this->pimpl_->select(rank, sym);
}




template <typename CtrName, typename Profile>
bool IterApiBTFLBase<CtrName, Profile>::seq_is_end() const
{
    return this->pimpl_->is_end();
}


template <typename CtrName, typename Profile>
bool IterApiBTFLBase<CtrName, Profile>::next_sym()
{
    return this->pimpl_->next();
}

template <typename CtrName, typename Profile>
bool IterApiBTFLBase<CtrName, Profile>::prev_sym()
{
    return this->pimpl_->prev();
}



template <typename CtrName, typename Profile>
int64_t IterApiBTFLBase<CtrName, Profile>::remove_entries(int64_t length)
{
    return this->pimpl_->remove_next(length);
}


#ifdef MMA1_USE_IOBUFFER
template <typename CtrName, typename Profile>
int64_t IterApiBTFLBase<CtrName, Profile>::read_seq(CtrIOBuffer& buffer, int64_t size) 
{
    auto walker = this->pimpl_->template create_read_walker<CtrIOBuffer>(size);
    
    return this->pimpl_->bulkio_populate(*walker.get(), &buffer);
}

template <typename CtrName, typename Profile>
int64_t IterApiBTFLBase<CtrName, Profile>::read_seq(bt::BufferConsumer<CtrIOBuffer>& consumer, int64_t size) 
{
    return this->pimpl_->bulkio_read(&consumer, size);
}




template <typename CtrName, typename Profile>
int64_t IterApiBTFLBase<CtrName, Profile>::insert_subseq(bt::BufferProducer<CtrIOBuffer>& producer) 
{
    return this->pimpl_->bulkio_insert(producer).sum();
}
#endif

template <typename CtrName, typename Profile>
int64_t IterApiBTFLBase<CtrName, Profile>::insert_subseq(
        io::IOVectorProducer& producer,
        int64_t start,
        int64_t length
)
{
    return this->pimpl_->bulkio_insert(producer, start, length).sum();
}



template <typename CtrName, typename Profile>
int64_t IterApiBTFLBase<CtrName, Profile>::seq_pos()
{
    return this->pimpl_->pos();
}

template <typename CtrName, typename Profile>
int64_t IterApiBTFLBase<CtrName, Profile>::skip_seq(int64_t offset)
{
    return this->pimpl_->skip(offset);
}



}
}
