
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

    using typename Base::TreeNodePtr;
    using typename Base::TreeNodeConstPtr;
    using typename Base::Iterator;
    using typename Base::IteratorPtr;
    using typename Base::Position;
    using typename Base::CtrSizeT;
    using typename Base::TreePathT;
    using typename Base::ProfileT;
    using typename Base::BlockID;
    using typename Base::CtrID;
    using typename Base::ROAllocatorPtr;

    class CtrBlockImpl: public CtrBlock<ApiProfile<ProfileT>> {
        ROAllocatorPtr store_;
        CtrID ctr_id_;
        BlockID block_id_;
    public:
        CtrBlockImpl(
                const ROAllocatorPtr& store,
                CtrID ctr_id,
                BlockID block_id
        ):
            store_(store),
            ctr_id_(ctr_id),
            block_id_(block_id)
        {}

        virtual bool is_leaf() const
        {
            auto block = static_cast_block<TreeNodeConstPtr>(store_->getBlock(block_id_));
            return block->is_leaf();
        }

        virtual AnyID block_id() const
        {
            return block_id_.as_any_id();
        }

        virtual std::vector<CtrBlockPtr<ApiProfile<ProfileT>>> children() const
        {
            std::vector<CtrBlockPtr<ApiProfile<ProfileT>>> children;

            auto block = static_cast_block<TreeNodeConstPtr>(store_->getBlock(block_id_));

            if (!block->is_leaf())
            {
                with_ctr([&](auto& ctr)  {
                    return ctr.ctr_for_all_ids(block, [&](const BlockID& child_id) {

                        children.emplace_back(
                            std::make_shared<CtrBlockImpl>(
                                store_,
                                ctr_id_,
                                child_id
                            )
                        );
                    });
                });
            }

            return children;
        }

        virtual void describe(std::ostream& out)  const
        {
            auto block = store_->getBlock(block_id_);

            return with_ctr([&](auto& ctr) {
                return ctr.ctr_dump_node(std::move(block), out);
            });
        }

    private:
        void with_ctr(
                std::function<void (MyType&)> fn
        ) const
        {
            auto ctr_ref = store_->find(ctr_id_);
            if (ctr_ref)
            {
                auto ctr_ptr = memoria_static_pointer_cast<MyType>(ctr_ref);
                return fn(*ctr_ptr);
            }
            else {
                MEMORIA_MAKE_GENERIC_ERROR("No container is found for id {}", ctr_id_).do_throw();
            }
        }
    };

    virtual CtrBlockPtr<ApiProfile<ProfileT>> root_block()
    {
        auto& self = this->self();
        return std::make_shared<CtrBlockImpl>(
            self.store().self_ptr(),
            self.name(),
            self.root()
        );
    }

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::FindName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS




#undef M_TYPE
#undef M_PARAMS

}
