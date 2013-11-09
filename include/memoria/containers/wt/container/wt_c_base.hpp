
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CONTAINERS_WT_C_BASE_HPP
#define MEMORIA_CONTAINERS_WT_C_BASE_HPP



#include <memoria/core/container/container.hpp>
#include <memoria/core/container/names.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>
#include <memoria/core/types/algo.hpp>
#include <memoria/core/tools/static_array.hpp>

#include <memoria/containers/wt/wt_names.hpp>

#include <iostream>

namespace memoria       {
namespace wt            {

MEMORIA_BT_MODEL_BASE_CLASS_NO_CTOR_BEGIN(WTCtrBase)

    typedef TypesType                                                           Types;
    typedef typename Types::Profile                                             Profile;

    typedef typename Base::ID                                                   ID;
    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::CtrShared                                            CtrShared;

    typedef WTLabeledTree<
                FLabel<UByte>,
                VLabel<BigInt,
                    Granularity::Bit,
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
        seq_. initCtr(&tree_, 0, command);

        Base::setCtrShared(NULL);
    }

    void initCtr(const ID& root_id, BigInt name)
    {
        auto& self = this->self();

        tree_.initCtr(&self.allocator(), root_id, name);
        seq_.initCtr(&tree_, get_ctr_root(self.allocator(), root_id, name, 0), 0);

        Base::setCtrShared(NULL);
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
        }

        return hash;
    }

    virtual ID getRootID(BigInt name)
    {
        return self().tree().getRootID(name);
    }




    virtual void setRoot(BigInt name, const ID& root_id)
    {
        self().tree().setRoot(name, root_id);
    }

    bool check(void* ptr = NULL)
    {
        bool tree_errors   = tree_.check(ptr);
        bool seq_errors     = seq_.check(ptr);

        return tree_errors || seq_errors;
    }

    const CtrShared* shared() const {
        return tree_.shared();
    }

    CtrShared* shared() {
        return tree_.shared();
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

    static ID get_ctr_root(Allocator& allocator, const ID& root_id, BigInt ctr_name, BigInt name)
    {
        typedef typename Tree::NodeBaseG   NodeBaseG;
        typedef typename Tree::Metadata    Metadata;

        NodeBaseG root  = allocator.getPage(root_id, Allocator::READ, ctr_name);
        Metadata  meta  = Tree::getCtrRootMetadata(root);

        return meta.roots(name);
    }

MEMORIA_BT_MODEL_BASE_CLASS_END


}}

#endif
