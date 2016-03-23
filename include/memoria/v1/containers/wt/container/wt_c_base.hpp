
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

#include <memoria/v1/containers/wt/wt_names.hpp>

#include <iostream>

namespace memoria {
namespace v1 {
namespace wt            {

MEMORIA_V1_BT_MODEL_BASE_CLASS_NO_CTOR_BEGIN(WTCtrBase)

    typedef TypesType                                                           Types;
    typedef typename Types::Profile                                             Profile;

    typedef typename Base::ID                                                   ID;
    typedef typename Base::Allocator                                            Allocator;


    typedef WTLabeledTree<
                FLabel<UByte>,
                VLabel<
                    BigInt,
                    Granularity::Byte,
                    Indexed::Yes
                >
    >                                                                           TreeName;

    typedef Sequence<8, true>                                                   SeqName;

    typedef typename CtrTF<Profile, TreeName, TreeName>::Type                   Tree;
    typedef typename CtrTF<Profile, SeqName, SeqName>::Type                     Seq;

private:
    Tree   tree_;
    Seq    seq_;

public:

    WTCtrBase(const CtrInitData& data):
        Base(data),
        tree_(data.owner(Base::CONTAINER_HASH)),
        seq_(data.owner(Base::CONTAINER_HASH))
    {}

    WTCtrBase(const ThisType& other, Allocator* allocator):
        Base(other, allocator),
        tree_(other.tree_, allocator),
        seq_(other.seq_, allocator)
    {}

    WTCtrBase(ThisType&& other, Allocator* allocator):
        Base(std::move(other), allocator),
        tree_(std::move(other.tree_), allocator),
        seq_(std::move(other.seq_), allocator)
    {}

    //broken constructor
    WTCtrBase(const ThisType& other):
        Base(other),
        tree_(NoParamCtr()),
        seq_(NoParamCtr())
    {}

    WTCtrBase(ThisType&& other):
        Base(std::move(other)),
        tree_(NoParamCtr()),
        seq_(NoParamCtr())
    {}

    Seq& seq() {
        return seq_;
    }

    Tree& tree() {
        return tree_;
    }

    const Seq& seq() const {
        return seq_;
    }

    const Tree& tree() const {
        return tree_;
    }

    void operator=(ThisType&& other)
    {
        Base::operator=(std::move(other));

        tree_  = std::move(other.tree_);
        seq_    = std::move(other.seq_);

    }

    void operator=(const ThisType& other)
    {
        Base::operator=(other);

        tree_   = other.tree_;
        seq_    = other.seq_;

    }

    void initCtr(Int command)
    {
        auto& self = this->self();

        tree_.initCtr(&self.allocator(), self.master_name(), command);
        seq_. initCtr(&tree_, UUID(), command);
    }

    void initCtr(const ID& root_id, const UUID& name)
    {
        auto& self = this->self();

        tree_.initCtr(&self.allocator(), root_id);
        seq_.initCtr(&tree_, get_ctr_root(self.allocator(), root_id, name, 0));
    }

    virtual bool hasRoot(const UUID& name)
    {
        throw Exception(MA_SRC, "Allocator::hasRoot(UUID) method must be properly implements for this container");
    }

    static Int initMetadata()
    {
        Int hash = Tree::initMetadata() + Seq::initMetadata();

        if (Base::getMetadata() == NULL)
        {
            MetadataList list;

            Tree::getMetadata()->putAll(list);
            Seq::getMetadata()->putAll(list);

            Base::setMetadata(new ContainerMetadata(
                                    TypeNameFactory<typename Types::ContainerTypeName>::name(),
                                    list,
                                    TypeHash<typename Types::ContainerTypeName>::Value,
                                    Base::getContainerInterface()
                             )
            );

            MetadataRepository<typename Types::Profile>::registerMetadata(Base::getMetadata());
        }

        return hash;
    }

    virtual ID getRootID(const UUID& name)
    {
        return self().tree().getRootID(name);
    }




    virtual void setRoot(const UUID& name, const ID& root_id)
    {
        self().tree().setRoot(name, root_id);
    }

    bool check(void* ptr = NULL)
    {
        bool tree_errors   = tree_.check(ptr);
        bool seq_errors     = seq_.check(ptr);

        return tree_errors || seq_errors;
    }

    void checkIt()
    {
        tree().checkIt();
        seq().checkIt();
    }

    void walkTree(ContainerWalker* walker)
    {
        auto& self = this->self();

        walker->beginCompositeCtr(
                TypeNameFactory<typename Types::ContainerTypeName>::name().c_str(),
                self.name()
        );

        tree_.walkTree(walker);
        seq_.walkTree(walker);

        walker->endCompositeCtr();
    }

private:

    static ID get_ctr_root(Allocator& allocator, const ID& root_id, const UUID& ctr_name, BigInt name)
    {
        typedef typename Tree::NodeBaseG   NodeBaseG;
        typedef typename Tree::Metadata    Metadata;

        NodeBaseG root  = allocator.getPage(root_id, ctr_name);
        Metadata  meta  = Tree::getCtrRootMetadata(root);

        return meta.roots(UUID(0, name));
    }

MEMORIA_V1_BT_MODEL_BASE_CLASS_END


}}}