
// Copyright 2020 Victor Smirnov
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

#include <memoria/prototypes/bt/bt_names.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>

#include <memoria/core/container/macros.hpp>

#include <limits>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(bt::BlockName)
public:
    using Types = TypesType;

    using Allocator = typename Base::Allocator;

    using typename Base::TreeNodePtr;
    using typename Base::Iterator;
    using typename Base::IteratorPtr;
    using typename Base::Position;
    using typename Base::CtrSizeT;
    using typename Base::TreePathT;
    using typename Base::ProfileT;
    using typename Base::BlockID;
    using typename Base::CtrID;
    using typename Base::AllocatorPtr;

    class CtrBlockImpl: public CtrBlock<ApiProfile<ProfileT>> {
        AllocatorPtr store_;
        CtrID ctr_id_;
        BlockID block_id_;
    public:
        CtrBlockImpl(
                const AllocatorPtr& store,
                CtrID ctr_id,
                BlockID block_id
        ) noexcept:
            store_(store),
            ctr_id_(ctr_id),
            block_id_(block_id)
        {}

        virtual BoolResult is_leaf() const noexcept
        {
            MEMORIA_TRY(block, static_cast_block<TreeNodePtr>(store_->getBlock(block_id_)));
            return BoolResult::of(block->is_leaf());
        }

        virtual ApiProfileBlockID<ApiProfile<ProfileT>> block_id() const noexcept
        {
            return block_id_holder_from(block_id_);
        }

        virtual Result<std::vector<CtrBlockPtr<ApiProfile<ProfileT>>>> children() const noexcept
        {
            using ResultT = Result<std::vector<CtrBlockPtr<ApiProfile<ProfileT>>>>;

            std::vector<CtrBlockPtr<ApiProfile<ProfileT>>> children;

            MEMORIA_TRY(block, static_cast_block<TreeNodePtr>(store_->getBlock(block_id_)));

            if (!block->is_leaf())
            {
                MEMORIA_TRY_VOID(with_ctr([&](auto& ctr) -> VoidResult {
                    return ctr.ctr_for_all_ids(block, [&](const BlockID& child_id) -> VoidResult {

                        children.emplace_back(
                            std::make_shared<CtrBlockImpl>(
                                store_,
                                ctr_id_,
                                child_id
                            )
                        );

                        return VoidResult::of();
                    });
                }));
            }

            return ResultT::of(children);
        }

        virtual VoidResult describe(std::ostream& out)  const noexcept
        {
            MEMORIA_TRY(block, store_->getBlock(block_id_));

            return with_ctr([&](auto& ctr) -> VoidResult {
                return ctr.ctr_dump_node(std::move(block), out);
            });
        }

    private:
        VoidResult with_ctr(
                std::function<VoidResult (MyType&)> fn
        ) const noexcept
        {
            MEMORIA_TRY(ctr_ref, store_->find(ctr_id_));
            if (ctr_ref)
            {
                auto ctr_ptr = memoria_static_pointer_cast<MyType>(ctr_ref);
                return fn(*ctr_ptr);
            }
            else {
                return MEMORIA_MAKE_GENERIC_ERROR("No container is found for id {}", ctr_id_);
            }
        }
    };

    virtual Result<CtrBlockPtr<ApiProfile<ProfileT>>> root_block() noexcept
    {
        using ResultT = Result<CtrBlockPtr<ApiProfile<ProfileT>>>;
        auto& self = this->self();


        return ResultT::of(std::make_shared<CtrBlockImpl>(
            self.store().self_ptr(),
            self.name(),
            self.root()
        ));
    }

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::FindName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS




#undef M_TYPE
#undef M_PARAMS

}
