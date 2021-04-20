
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

#include <memoria/api/common/ctr_api_btfl.hpp>
#include <memoria/api/db/update_log/update_log_output.hpp>

#include <memoria/core/tools/uuid.hpp>

#include <memoria/core/tools/object_pool.hpp>

#include <memoria/core/datatypes/traits.hpp>

#include <memoria/core/types/typehash.hpp>


#include <memory>
#include <tuple>

namespace memoria {

struct UpdateLog {};

template <typename Profile>
class CtrApi<UpdateLog, Profile>: public CtrApiBTFLBase<UpdateLog, Profile> {
    
    using Base = CtrApiBTFLBase<UpdateLog, Profile>;
    using typename Base::StoreT;
    using typename Base::CtrT;
    using typename Base::CtrPtr;

    using typename Base::Iterator;
    
public:
    using typename Base::CtrID;
    using typename Base::CtrSizeT;
    
    static constexpr int32_t DataStreams = 3;
    using CtrSizesT = ProfileCtrSizesT<Profile, DataStreams + 1>;
    
    using CommandsDataIteratorT = update_log::CommandsDataIterator<Iterator, CtrSizeT>;
    using SnapshotIDIteratorT   = update_log::SnapshotIDIterator<Iterator, CtrSizeT>;

    MMA_DECLARE_CTRAPI_BASIC_METHODS()

    int64_t size() const;

    void create_snapshot(const UUID& snapshot_id);

    CommandsDataIteratorT read_commads(const UUID& ctr_name, CtrSizeT start = 0);

    bool remove_commands(const UUID& ctr_name, CtrSizeT start, CtrSizeT length);
    bool remove_commands(const UUID& ctr_name);

    SnapshotIDIteratorT find_snapshot(const UUID& snapshot_id);

    SnapshotIDIteratorT latest_snapshot();
};


template <typename Profile>
class IterApi<UpdateLog, Profile>: public IterApiBTFLBase<UpdateLog, Profile> {
    
    using Base = IterApiBTFLBase<UpdateLog, Profile>;
    
    using typename Base::IterT;
    using typename Base::IterPtr;
    
public:
    using typename Base::CtrSizeT;
    
    static constexpr int32_t DataStreams = CtrApi<UpdateLog, Profile>::DataStreams;
    using CtrSizesT = typename CtrApi<UpdateLog, Profile>::CtrSizesT;
    
    MMA_DECLARE_ITERAPI_BASIC_METHODS()

    int32_t leaf_pos(int32_t data_stream);
};


template <>
struct TypeHash<UpdateLog>: UInt64Value <
    HashHelper<4001>
> {};

template <>
struct DataTypeTraits<UpdateLog> {
    using CxxType   = EmptyType;
    using InputView = EmptyType;
    using Ptr       = EmptyType*;

    static constexpr bool HasTypeConstructors = false;

    static void create_signature(SBuf& buf, const UpdateLog& obj) {
        buf << "UpdateLog";
    }

    static void create_signature(SBuf& buf) {
        buf << "UpdateLog";
    }
};
    
}
