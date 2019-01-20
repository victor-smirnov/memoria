
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

#include <memoria/v1/core/container/container.hpp>
#include <memoria/v1/core/container/names.hpp>
#include <memoria/v1/prototypes/bt/bt_macros.hpp>
#include <memoria/v1/core/types/algo.hpp>
#include <memoria/v1/core/tools/static_array.hpp>

#include <memoria/v1/containers/vector_tree/vtree_names.hpp>

#include <iostream>

namespace memoria {
namespace v1 {
namespace vtree         {

MEMORIA_V1_BT_MODEL_BASE_CLASS_NO_CTOR_BEGIN(VTreeCtrBase)
public:
    typedef TypesType                                                           Types;
    typedef typename Types::Profile                                             Profile;

    typedef typename Base::ID                                                   ID;
    typedef typename Base::Allocator                                            Allocator;

    using TreeName = LabeledTree<
                FLabel<int64_t>,
                VLabel<int64_t,
                    Granularity::int8_t,
                    Indexed::Yes
                >
    >;

    using VectorName = Vector<int16_t>;

    typedef typename CtrTF<Profile, TreeName, TreeName>::Type                   Tree;
    typedef typename CtrTF<Profile, VectorName, VectorName>::Type               Vec;
    typedef typename Vec::Types::Value                                          Value;

    using TreePtr   = std::shared_ptr<Tree>;
    using VecPtr    = std::shared_ptr<Vec>;

    using NodeBaseG = typename Tree::Types::NodeBaseG;
    using Metadata  = typename Tree::Types::Metadata;

private:
    TreePtr   tree_;
    VecPtr    vector_;

public:

    VTreeCtrBase(const CtrInitData& data):
        Base(data),
        tree_(std::make_shared<Tree>(data.owner(Base::CONTAINER_HASH))),
        vector_(std::make_shared<Vec>(data.owner(Base::CONTAINER_HASH)))
    {}

    VecPtr& vector() {
        return vector_;
    }

    TreePtr& tree() {
        return tree_;
    }

    const VecPtr& vector() const {
        return vector_;
    }

    const TreePtr& tree() const {
        return tree_;
    }


    void initCtr(int32_t command)
    {
        auto& self = this->self();

        tree_->initCtr(&self.allocator(), self.master_name(), command);
        vector_->initCtr(tree_.get(), UUID{}, command);
    }

    void initCtr(const ID& root_id, UUID name)
    {
        auto& self = this->self();

        tree_->initCtr(&self.allocator(), root_id);
        vector_->initCtr(tree_.get(), get_ctr_root(self.allocator(), root_id, name, 0));
    }

    virtual bool hasRoot(const UUID& name)
    {
        throw Exception(MA_SRC, "Allocator::hasRoot(UUID) method must be properly implements for this container");
    }


    static int32_t initMetadata()
    {
        int32_t hash = Tree::initMetadata() + Vec::initMetadata();

        if (!Base::getMetadata())
        {
            MetadataList list;

            Tree::getMetadata()->putAll(list);
            Vec::getMetadata()->putAll(list);

            Base::setMetadata(std::make_shared<ContainerMetadata>(
                    TypeNameFactory<typename Types::ContainerTypeName>::name(),
                    list,
                    static_cast<uint64_t>(TypeHash<typename Types::ContainerTypeName>::Value),
                    Base::getContainerInterface()
            )
            );

            MetadataRepository<typename Types::Profile>::registerMetadata(Base::getMetadata());
        }

        return hash;
    }

    virtual ID getRootID(const UUID& name)
    {
        return self().tree()->getRootID(name);
    }




    virtual void setRoot(const UUID& name, const ID& root_id)
    {
        self().tree()->setRoot(name, root_id);
    }

    bool check(void* ptr = NULL)
    {
        bool tree_errors   = tree_->check(ptr);
        bool seq_errors     = vector_->check(ptr);

        return tree_errors || seq_errors;
    }

    void walkTree(ContainerWalker* walker)
    {
        auto& self = this->self();

        walker->beginCompositeCtr(
                TypeNameFactory<typename Types::ContainerTypeName>::name().c_str(),
                self.master_name()
        );

        tree_->walkTree(walker);
        vector_->walkTree(walker);

        walker->endCompositeCtr();
    }

    void drop() {
        tree_->drop();
    }

    static auto getModelNameS(NodeBaseG root)
    {
        return getRootMetadataS(root).model_name();
    }

    static const auto& getRootMetadataS(NodeBaseG node)
    {
        MEMORIA_V1_ASSERT_TRUE(node.isSet());
        MEMORIA_V1_ASSERT_TRUE(node->is_root());

        return node->root_metadata();
    }

private:

    static ID get_ctr_root(Allocator& allocator, const ID& root_id, const UUID& ctr_name, const int64_t& name)
    {
        typedef typename Tree::NodeBaseG   NodeBaseG;
        typedef typename Tree::Metadata    Metadata;

        NodeBaseG root  = allocator.getBlock(root_id, ctr_name);
        Metadata  meta  = Tree::getCtrRootMetadata(root);

        return meta.roots(UUID(0, name));
    }

MEMORIA_V1_BT_MODEL_BASE_CLASS_END


}}}
