
// Copyright 2013 Victor Smirnov
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


#include <memoria/containers/vector/vctr_names.hpp>

#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>


#include <memoria/api/vector/vector_producer.hpp>
#include <memoria/api/vector/vector_scanner.hpp>
#include <memoria/api/vector/vector_api.hpp>


namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(mvector::CtrApiCommonName)

public:
    using Types = typename Base::Types;

protected:
    using typename Base::NodeBaseG;
    using typename Base::Position;
    using typename Base::BranchNodeEntry;
    using typename Base::BlockUpdateMgr;
    using typename Base::CtrSizeT;
    using typename Base::IteratorPtr;
    using typename Base::Profile;

    using Value = typename Types::Value;
    using ValueDataType = typename Types::ValueDataType;
    using ViewType  = DTTViewType<ValueDataType>;
    using CtrApiTypes = ICtrApiTypes<typename Types::ContainerTypeName, Profile>;

    using BufferT = DataTypeBuffer<ValueDataType>;

public:

    using typename Base::BranchNodeExtData;
    using typename Base::LeafNodeExtData;
    using typename Base::ContainerTypeName;

    void configure_types(
        const ContainerTypeName& type_name,
        BranchNodeExtData& branch_node_ext_data,
        LeafNodeExtData& leaf_node_ext_data
    ) {

    }

    virtual VoidResult read_to(BufferT& buffer, CtrSizeT start, CtrSizeT length) const noexcept
    {
        using ResultT = VoidResult;
        auto& self = this->self();

        MEMORIA_TRY(ii, self.ctr_seek(start));

        CtrSizeT cnt{};
        VectorScanner<CtrApiTypes, Profile> scanner(ii);

        size_t local_cnt;
        while (cnt < length && !scanner.is_end())
        {
            local_cnt = 0;
            CtrSizeT remainder   = length - cnt;
            CtrSizeT values_size = static_cast<CtrSizeT>(scanner.values().size());

            if (values_size <= remainder)
            {
                buffer.append(scanner.values());
                cnt += values_size;
            }
            else {
                buffer.append(scanner.values().first(remainder));
                cnt += remainder;
            }

            if (cnt < length)
            {
                MEMORIA_TRY_VOID(scanner.next_leaf());
            }
        }

        return ResultT::of();
    }

    virtual VoidResult insert(CtrSizeT at, const BufferT& buffer, size_t start, size_t size) noexcept
    {
        auto& self = this->self();

        if (start + size > buffer.size())
        {
            return VoidResult::make_error("Vector insert_buffer range check error: {}, {}, {}", start, size, buffer.size());
        }

        VectorProducer<CtrApiTypes> producer([&](auto& values, auto appended_size){
            size_t batch_size = 8192;
            size_t limit = (appended_size + batch_size <= size) ? batch_size : size - appended_size;

            for (size_t c = 0; c < limit; c++) {
                values.append(buffer[start + appended_size + c]);
            }

            return limit != batch_size;
        });

        MEMORIA_TRY(ii, self.ctr_seek(at));
        MEMORIA_TRY_VOID(ii->insert_iovector(producer, 0, std::numeric_limits<CtrSizeT>::max()));

        return VoidResult::of();
    }



    Result<ProfileCtrSizeT<Profile>> size() const noexcept
    {
        using ResultT = Result<ProfileCtrSizeT<Profile>>;
        auto res = self().sizes();
        MEMORIA_RETURN_IF_ERROR(res);

        return ResultT::of(res.get()[0]);
    }

    Result<CtrSharedPtr<VectorIterator<ValueDataType, Profile>>> seek(CtrSizeT pos) const noexcept
    {
        typename Types::template SkipForwardWalker<Types, IntList<0>> walker(pos);
        return memoria_static_pointer_cast<VectorIterator<ValueDataType, Profile>>(self().ctr_find(walker));
    }


    Result<Datum<Value>> get(CtrSizeT pos) const noexcept
    {
        auto ii = self().seek(pos);
        MEMORIA_RETURN_IF_ERROR(ii);

        return ii.get()->value();
    }

    VoidResult set(CtrSizeT pos, ViewType view) noexcept
    {
        auto ii = self().seek(pos);
        MEMORIA_RETURN_IF_ERROR(ii);

        return ii.get()->set(view);
    }

    VoidResult prepend(io::IOVectorProducer& producer) noexcept
    {
        auto ii = self().ctr_begin();
        MEMORIA_RETURN_IF_ERROR(ii);

        auto res = ii.get()->insert_iovector(producer, 0, std::numeric_limits<CtrSizeT>::max());
        MEMORIA_RETURN_IF_ERROR(res);

        return VoidResult::of();
    }

    VoidResult append(io::IOVectorProducer& producer) noexcept
    {
        auto ii = self().ctr_end();
        MEMORIA_RETURN_IF_ERROR(ii);

        auto res = ii.get()->insert_iovector(producer, 0, std::numeric_limits<CtrSizeT>::max());
        MEMORIA_RETURN_IF_ERROR(res);

        return VoidResult::of();
    }

    VoidResult insert(CtrSizeT at, io::IOVectorProducer& producer) noexcept
    {
        auto ii = self().ctr_seek(at);
        MEMORIA_RETURN_IF_ERROR(ii);

        auto res = ii.get()->insert_iovector(producer, 0, std::numeric_limits<CtrSizeT>::max());
        MEMORIA_RETURN_IF_ERROR(res);
        return VoidResult::of();
    }

    Result<CtrSizeT> remove(CtrSizeT from, CtrSizeT to) noexcept
    {
        auto& self = this->self();

        auto ii_from = self.ctr_seek(from);
        MEMORIA_RETURN_IF_ERROR(ii_from);

        return ii_from.get()->remove_from(to - from);
    }

    Result<CtrSizeT> remove_from(CtrSizeT from) noexcept
    {
        auto res = size();
        MEMORIA_RETURN_IF_ERROR(res);

        return remove(from, res.get());
    }

    Result<CtrSizeT> remove_up_to(CtrSizeT pos) noexcept
    {
        auto& self = this->self();

        auto ii_from = self.ctr_begin();
        MEMORIA_RETURN_IF_ERROR(ii_from);

        return ii_from.get()->remove_from(pos);
    }


MEMORIA_V1_CONTAINER_PART_END

}
