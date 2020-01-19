
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

#include <memoria/v1/api/common/ctr_api_btss.hpp>

#include <memoria/v1/api/vector/vector_input.hpp>

#include <memoria/v1/core/tools/span.hpp>

#include <memoria/v1/core/types/typehash.hpp>
#include <memoria/v1/core/datatypes/traits.hpp>

#include <memoria/v1/api/vector/vector_api_factory.hpp>
#include <memoria/v1/api/vector/vector_producer.hpp>
#include <memoria/v1/api/vector/vector_scanner.hpp>

#include <memory>
#include <vector>

namespace memoria {
namespace v1 {

template <typename DataType, typename Profile>
struct VectorIterator: BTSSIterator<Profile> {

    using ViewType  = DTTViewType<DataType>;
    using CtrSizeT  = ProfileCtrSizeT<Profile>;

    virtual Datum<DataType> value() const = 0;
    virtual VoidResult set(ViewType view) noexcept = 0;
    virtual BoolResult next() noexcept = 0;

    virtual Result<CtrSizeT> remove_from(CtrSizeT size) noexcept = 0;
};

template <typename DataType, typename Profile, bool FixedSizeElement = DTTIs1DFixedSize<DataType>>
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

    using CtrSizeT  = ProfileCtrSizeT<Profile>;

    using Producer      = VectorProducer<ApiTypes>;
    using ProducerFn    = typename Producer::ProducerFn;


    virtual Result<Datum<DataType>> get(CtrSizeT pos) const = 0;
    virtual VoidResult set(CtrSizeT pos, ViewType view) noexcept = 0;

    virtual Result<CtrSizeT> size() const noexcept = 0;

    virtual Result<CtrSharedPtr<VectorIterator<DataType, Profile>>> seek(CtrSizeT pos) const = 0;

    virtual VoidResult prepend(io::IOVectorProducer& producer) noexcept = 0;
    virtual VoidResult append(io::IOVectorProducer& producer) noexcept = 0;
    virtual VoidResult insert(CtrSizeT at, io::IOVectorProducer& producer) noexcept = 0;

    template <typename Fn>
    VectorScanner<ApiTypes, Profile> as_scanner(Fn&& fn) const {
        return VectorScanner<ApiTypes, Profile>(fn(this));
    }

    VoidResult append(ProducerFn producer_fn) noexcept {
        Producer producer(producer_fn);
        return append(producer);
    }

    VoidResult prepend(ProducerFn producer_fn) noexcept {
        Producer producer(producer_fn);
        return prepend(producer);
    }

    VoidResult insert(CtrSizeT at, ProducerFn producer_fn) noexcept {
        Producer producer(producer_fn);
        return insert(at, producer);
    }

    VoidResult insert(CtrSizeT at, Span<const ViewType> span) noexcept
    {
        return insert(at, [&](auto& values, size_t){
            values.append(span);
            return false;
        });
    }

    virtual Result<CtrSizeT> remove(CtrSizeT from, CtrSizeT to) noexcept = 0;
    virtual Result<CtrSizeT> remove_from(CtrSizeT from) noexcept = 0;
    virtual Result<CtrSizeT> remove_up_to(CtrSizeT pos) noexcept = 0;

    MMA1_DECLARE_ICTRAPI();
};



}}
