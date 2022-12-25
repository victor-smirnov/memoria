
// Copyright 2017-2022 Victor Smirnov
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

#include <memoria/api/common/ctr_api_btss.hpp>
#include <memoria/api/common/ctr_input_btss.hpp>

#include <memoria/core/tools/span.hpp>

#include <memoria/core/reflection/typehash.hpp>
#include <memoria/core/datatypes/traits.hpp>

#include <memoria/api/collection/collection_api.hpp>

#include <memoria/api/vector/vector_api_factory.hpp>

#include <memory>
#include <vector>

namespace memoria {
    
template <typename DataType, typename Profile>
struct ICtrApi<Vector<DataType>, Profile>: public ICtrApi<Collection<DataType>, Profile> {

    using ApiTypes  = ICtrApiTypes<Vector<DataType>, Profile>;

    using ViewType  = DTTViewType<DataType>;

    using CtrSizeT  = ApiProfileCtrSizeT<Profile>;

    using DataTypeT     = DataType;

    using CtrInputBuffer = typename ApiTypes::CtrInputBuffer;
    using ChunkIteratorPtr = IterSharedPtr<CollectionChunk<DataType, Profile>>;

    virtual void read_to(CtrInputBuffer& buffer, CtrSizeT start, CtrSizeT length) const = 0;
    virtual ChunkIteratorPtr insert(CtrSizeT at, CtrInputBuffer& buffer) MEMORIA_READ_ONLY_API

    virtual Datum<DataType> get(CtrSizeT pos) const = 0;
    virtual void set(CtrSizeT pos, const ViewType& view) MEMORIA_READ_ONLY_API

    virtual ChunkIteratorPtr prepend(CtrBatchInputFn<CtrInputBuffer> producer) MEMORIA_READ_ONLY_API
    virtual ChunkIteratorPtr append(CtrBatchInputFn<CtrInputBuffer> producer) MEMORIA_READ_ONLY_API
    virtual ChunkIteratorPtr insert(CtrSizeT at, CtrBatchInputFn<CtrInputBuffer> producer) MEMORIA_READ_ONLY_API

    ChunkIteratorPtr insert(CtrSizeT at, Span<const ViewType> span)
    {
        return insert(at, [&](auto& values){
            values.append(span);
            return true;
        });
    }

//    virtual CtrSizeT remove(CtrSizeT from, CtrSizeT to) MEMORIA_READ_ONLY_API
//    virtual CtrSizeT remove_from(CtrSizeT from) MEMORIA_READ_ONLY_API
//    virtual CtrSizeT remove_up_to(CtrSizeT pos) MEMORIA_READ_ONLY_API

    MMA_DECLARE_ICTRAPI();
};

}
