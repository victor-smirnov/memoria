
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

#include <memoria/core/types/typehash.hpp>
#include <memoria/core/datatypes/traits.hpp>

#include <memoria/api/vector/vector_api_factory.hpp>

#include <memory>
#include <vector>

namespace memoria {

template <typename DataType, typename Profile, bool FixedSizeElement = DTTIsNDFixedSize<DataType>>
struct VectorApiBase;

template <typename DataType, typename Profile>
struct VectorApiBase<DataType, Profile, true>: public CtrReferenceable<Profile> {

};

template <typename DataType, typename Profile>
struct VectorApiBase<DataType, Profile, false>: public CtrReferenceable<Profile> {

};
    
template <typename DataType, typename Profile>
struct ICtrApi<Vector<DataType>, Profile>: public VectorApiBase<DataType, Profile> {

    using ApiTypes  = ICtrApiTypes<Vector<DataType>, Profile>;

    using ViewType  = DTTViewType<DataType>;

    using CtrSizeT  = ApiProfileCtrSizeT<Profile>;

    using BufferT       = DataTypeBuffer<DataType>;
    using DataTypeT     = DataType;

    using CtrInputBuffer = typename ApiTypes::CtrInputBuffer;

    virtual void read_to(BufferT& buffer, CtrSizeT start, CtrSizeT length) const = 0;
    virtual void insert(CtrSizeT at, const BufferT& buffer, size_t start, size_t length) = 0;

    virtual void insert(CtrSizeT at, const BufferT& buffer) {
        return insert(at, buffer, 0, buffer.size());
    }


    virtual Datum<DataType> get(CtrSizeT pos) const = 0;
    virtual void set(CtrSizeT pos, ViewType view) = 0;

    virtual CtrSizeT size() const = 0;

    //virtual IteratorT seek(CtrSizeT pos) const = 0;

    virtual void prepend(CtrBatchInputFn<CtrInputBuffer> producer) = 0;
    virtual void append(CtrBatchInputFn<CtrInputBuffer> producer) = 0;
    virtual void insert(CtrSizeT at, CtrBatchInputFn<CtrInputBuffer> producer) = 0;


    void insert(CtrSizeT at, Span<const ViewType> span)
    {
        return insert(at, [&](auto& values){
            values.append(span);
            return true;
        });
    }

    virtual CtrSizeT remove(CtrSizeT from, CtrSizeT to) = 0;
    virtual CtrSizeT remove_from(CtrSizeT from) = 0;
    virtual CtrSizeT remove_up_to(CtrSizeT pos) = 0;

    MMA_DECLARE_ICTRAPI();
};

}
