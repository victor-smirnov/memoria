
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

#include <memoria/core/types.hpp>
#include <memoria/core/tools/dump.hpp>

#include <memoria/containers/vector/vctr_names.hpp>
#include <memoria/containers/vector/vctr_tools.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/prototypes/bt/bt_macros.hpp>

#include <memoria/api/vector/vector_producer.hpp>
#include <memoria/api/vector/vector_scanner.hpp>


#include <iostream>


namespace memoria {

MEMORIA_V1_ITERATOR_PART_BEGIN(mvector::ItrApiName)

    typedef Ctr<typename Types::CtrTypes>                                       Container;

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::TreeNodePtr                                            TreeNodePtr;

    typedef typename Types::ValueDataType                                       ValueDataType;
    typedef typename Container::BranchNodeEntry                                 BranchNodeEntry;

    typedef typename Container::Position                                        Position;

    using Profile  = typename Types::Profile;
    using ApiProfileT = ApiProfile<Profile>;

    using CtrSizeT = ProfileCtrSizeT<Profile>;


    using CtrApi = ICtrApi<typename Types::ContainerTypeName, ApiProfileT>;

    using CtrApiTypes = ICtrApiTypes<typename Types::ContainerTypeName, ApiProfileT>;

    using IOVSchema = Linearize<typename CtrApiTypes::IOVSchema>;

    using ValueView = typename DataTypeTraits<ValueDataType>::ViewType;

    using BufferT = DataTypeBuffer<ValueDataType>;

public:



    virtual CtrSharedPtr<BufferT> read_buffer(CtrSizeT size)
    {
        auto& self = this->self();

        auto buffer = ctr_make_shared<BufferT>();

        CtrSizeT cnt{};
        VectorScanner<CtrApiTypes, ApiProfileT> scanner(self.shared_from_this());

        size_t local_cnt;
        while (cnt < size && !scanner.is_end())
        {
            local_cnt = 0;
            CtrSizeT remainder = size - cnt;
            CtrSizeT values_size = static_cast<CtrSizeT>(scanner.values().size());

            if (values_size <= remainder)
            {
                buffer->append(scanner.values());
                cnt += values_size;
            }
            else {
                buffer->append(scanner.values().first(remainder));
                cnt += remainder;
            }

            if (cnt < size)
            {
                scanner.next_leaf();
            }
        }


        return buffer;
    }

    virtual void insert_buffer(const BufferT& buffer, size_t start, size_t size)
    {
        auto& self = this->self();

        if (start + size > buffer.size())
        {
            MEMORIA_MAKE_GENERIC_ERROR("Vector insert_buffer rancge check error: {}, {}, {}", start, size, buffer.size()).do_throw();
        }

        auto current_pos = self.pos();

        VectorProducer<CtrApiTypes> producer([&](auto& values, auto appended_size){
            size_t batch_size = 8192;
            size_t limit = (appended_size + batch_size <= size) ? batch_size : size - appended_size;

            for (size_t c = 0; c < limit; c++) {
                values.append(buffer[start + appended_size + c]);
            }

            return limit != batch_size;
        });

        auto totals = self.insert_iovector(producer, 0, std::numeric_limits<CtrSizeT>::max());

        CtrSizeT new_pos = current_pos + totals;

        auto ii = self.ctr().ctr_seek(new_pos);

        self.assign(std::move(*ii.get()));
    }


    CtrSharedPtr<CtrApi> vector() noexcept {
        return memoria_static_pointer_cast<CtrApi> (self().ctr().make_shared_ptr());
    }

    Datum<ValueDataType> value() const
    {
        auto& self = this->self();

        psize_t local_pos = self.iter_local_pos();

        if (local_pos < self.iter_leaf_size(0))
        {
            auto& iovv = self.iovector_view();

            using Adapter = IOSubstreamAdapter<Select<0, IOVSchema>>;

            ValueView value_view;

            Adapter::read_one(iovv.substream(0), 0, local_pos, value_view);

            return value_view;
        }
        else {
            MMA_THROW(BoundsException()
                       << format_ex(
                           "Requested index {} is outside of bounds [0, {})",
                           local_pos,
                           self.iter_leaf_size(0)
                       )
            );
        }
    }


    CtrSizeT seek(CtrSizeT pos)
    {
        auto& self = this->self();

        CtrSizeT current_pos = self.pos();
        self.skip(pos - current_pos);
    }

    struct EntryAdapter {
        ValueView view;
        template <int32_t StreamIdx, int32_t SubstreamIdx>
        ValueView get(bt::StreamTag<StreamIdx>, bt::StreamTag<SubstreamIdx>, int32_t block) const {
            return view;
        }
    };

    void set(ValueView v)
    {
        auto& self = this->self();
        psize_t local_pos = self.iter_local_pos();

        auto leaf_size = self.iter_leaf_size(0);
        if (local_pos < leaf_size)
        {
            return self.ctr().template ctr_update_entry<IntList<0, 1>>(self, EntryAdapter{v});
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR(
                "Requested index {} is outside of bounds [0, {})",
                local_pos,
                self.iter_leaf_size(0)
            ).do_throw();
        }
    }

MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(mvector::ItrApiName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS

#undef M_TYPE
#undef M_PARAMS

}
