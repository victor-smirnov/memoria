
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

#include <memoria/v1/containers/db/update_log/update_log_factory.hpp>

#include <memoria/v1/api/db/update_log/update_log_api.hpp>
#include <memoria/v1/api/db/update_log/update_log_input.hpp>

#include <memoria/v1/core/container/ctr_impl_btfl.hpp>
#include <memoria/v1/core/tools/static_array.hpp>

#include <memory>

namespace memoria {
namespace v1 {

template <typename Profile>
int64_t CtrApi<UpdateLog, Profile>::size() const
{
    return this->pimpl_->size();
}

template <typename Profile>
void CtrApi<UpdateLog, Profile>::create_snapshot(const UUID& snapshot_id)
{
    return this->pimpl_->create_snapshot(snapshot_id);
}

template <typename Profile>
void CtrApi<UpdateLog, Profile>::append_commands(const UUID& ctr_name, bt::BufferProducer<CtrIOBuffer>& data_producer)
{
    this->pimpl_->append_commands(ctr_name, data_producer);
}


template <typename Profile>
typename CtrApi<UpdateLog, Profile>::CommandsDataIteratorT
CtrApi<UpdateLog, Profile>::read_commads(const UUID& ctr_name, CtrSizeT start)
{
    return CommandsDataIteratorT{this->pimpl_->read_commads(ctr_name, start)};
}

template <typename Profile>
bool CtrApi<UpdateLog, Profile>::remove_commands(const UUID& ctr_name, CtrSizeT start, CtrSizeT length)
{
    return this->pimpl_->remove_commands(ctr_name, start, length);
}

template <typename Profile>
bool CtrApi<UpdateLog, Profile>::remove_commands(const UUID& ctr_name)
{
    return this->pimpl_->remove_commands(ctr_name);
}


template <typename Profile>
typename CtrApi<UpdateLog, Profile>::SnapshotIDIteratorT CtrApi<UpdateLog, Profile>::latest_snapshot()
{
    auto ii = this->pimpl_->latest_snapshot();
    return SnapshotIDIteratorT{ii, ii->pos(), ii->ctr().size()};
}


template <typename Profile>
typename CtrApi<UpdateLog, Profile>::SnapshotIDIteratorT
CtrApi<UpdateLog, Profile>::find_snapshot(const UUID& snapshot_id)
{
    auto ii = this->pimpl_->find_snapshot(snapshot_id);
    return SnapshotIDIteratorT{ii, ii->snapshot_id_run_pos(), ii->ctr().size()};
}


template <typename Iterator, typename CtrSizeT>
UUID update_log::SnapshotIDIterator<Iterator, CtrSizeT>::next()
{
    if (MMA1_LIKELY(has_next()))
    {
        auto vv = iterator_.ptr()->snapshot_id();

        iterator_.ptr()->selectGEFw(1, 0);
        pos_ ++;

        return vv;
    }

    MMA1_THROW(RuntimeException()) << WhatCInfo("No such element");
}

template <typename Iterator, typename CtrSizeT>
CtrSizeT update_log::SnapshotIDIterator<Iterator, CtrSizeT>::containers_size()
{
    return iterator_.ptr()->count_ctr_names();
}


template <typename Iterator, typename CtrSizeT>
typename update_log::SnapshotIDIterator<Iterator, CtrSizeT>::ContainerNameIteratorT
update_log::SnapshotIDIterator<Iterator, CtrSizeT>::containers()
{
    CtrSizeT ctr_names = iterator_.ptr()->count_ctr_names();

    if (ctr_names > 0)
    {
        auto ii = iterator_.clone();
        ii.ptr()->next();
        return ContainerNameIteratorT(ii, UAcc192T{}, 0, ctr_names);
    }
    else {
        return ContainerNameIteratorT{};
    }
}



template <typename Iterator, typename CtrSizeT>
UUID update_log::ContainerNameIterator<Iterator, CtrSizeT>::next()
{
    if (MMA1_LIKELY(has_next()))
    {
        prefix_ += iterator_.ptr()->ctr_name_i();

        iterator_.ptr()->selectGEFw(1, 1);
        pos_++;

        return prefix_.to_uuid();
    }

    MMA1_THROW(RuntimeException()) << WhatCInfo("No such element");
}

template <typename Iterator, typename CtrSizeT>
typename update_log::ContainerNameIterator<Iterator, CtrSizeT>::CommandsDataIteratorT
update_log::ContainerNameIterator<Iterator, CtrSizeT>::commands()
{
    auto ii = iterator_.clone();
    ii.ptr()->next();
    return CommandsDataIteratorT{ii};
}



template <typename Iterator, typename CtrSizeT>
void update_log::CommandsDataIterator<Iterator, CtrSizeT>::prepare()
{
    walker_ = iterator_.ptr()->make_command_data_walker();
    buffer_ = iterator_.ptr()->make_io_buffer();

    prefetch();
}


template <typename Iterator, typename CtrSizeT>
CtrSizeT update_log::CommandsDataIterator<Iterator, CtrSizeT>::pos()
{
    return iterator_.ptr()->data_run_pos();
}

template <typename Iterator, typename CtrSizeT>
CtrSizeT update_log::CommandsDataIterator<Iterator, CtrSizeT>::size()
{
    return iterator_.ptr()->data_run_size();
}

template <typename Iterator, typename CtrSizeT>
CtrSizeT update_log::CommandsDataIterator<Iterator, CtrSizeT>::seek(CtrSizeT pos)
{
    return iterator_.ptr()->data_seek(pos);
}

template <typename Iterator, typename CtrSizeT>
CtrSizeT update_log::CommandsDataIterator<Iterator, CtrSizeT>::remove(CtrSizeT size)
{
    return iterator_.ptr()->data_remove(size);
}



template <typename Iterator, typename CtrSizeT>
bool update_log::CommandsDataIterator<Iterator, CtrSizeT>::prefetch()
{
    if (walker_.get())
    {
        int32_t n_entries = walker_->populate(buffer_.get());

        buffer_->flip();

        entry_ = 0;
        if (MMA1_LIKELY(n_entries > 0))
        {
            empty_ = false;
            n_entries_ = n_entries;
        }
        else
        {
            empty_ = true;
            n_entries_ = -n_entries;
        }
    }
    else {
        empty_ = true;
    }

    return entry_ < n_entries_;
}







}}
