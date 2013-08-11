
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CONTAINERS_VTREE_C_BASE_HPP
#define MEMORIA_CONTAINERS_VTREE_C_BASE_HPP



#include <memoria/core/container/container.hpp>
#include <memoria/core/container/names.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>
#include <memoria/core/types/algo.hpp>
#include <memoria/core/tools/static_array.hpp>

#include <memoria/containers/vector_tree/vtree_names.hpp>

#include <iostream>

namespace memoria       {
namespace vtree    		{

MEMORIA_BT_MODEL_BASE_CLASS_NO_CTOR_BEGIN(VTreeCtrBase)

    typedef TypesType                                                           Types;
    typedef typename Types::Profile                                             Profile;

    typedef typename Base::ID                                                   ID;
    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::CtrShared                                            CtrShared;

    typedef LabeledTree<
    			FLabel<UByte>,
    			VLabel<BigInt,
    				Granularity::Bit,
    				Indexed::Yes
    			>
    >																			TreeName;

    typedef Vector<UByte>														VectorName;

    typedef typename CtrTF<Profile, TreeName, TreeName>::Type  					Tree;
    typedef typename CtrTF<Profile, VectorName, VectorName>::Type        		Vec;
    typedef typename Vec::Value													Value;

private:
    Tree   tree_;
    Vec    vector_;

public:

    VTreeCtrBase(const CtrInitData& data):
        Base(data),
        tree_(data.owner(Base::CONTAINER_HASH)),
        vector_(data.owner(Base::CONTAINER_HASH))
    {}

    VTreeCtrBase(const ThisType& other, Allocator* allocator):
        Base(other, allocator),
        tree_(other.tree_, allocator),
        vector_(other.vector_, allocator)
    {}

    VTreeCtrBase(ThisType&& other, Allocator* allocator):
        Base(std::move(other), allocator),
        tree_(std::move(other.tree_), allocator),
        vector_(std::move(other.vector_), allocator)
    {}

    //broken constructor
    VTreeCtrBase(const ThisType& other):
        Base(other),
        tree_(NoParamCtr()),
        vector_(NoParamCtr())
    {}

    VTreeCtrBase(ThisType&& other):
        Base(std::move(other)),
        tree_(NoParamCtr()),
        vector_(NoParamCtr())
    {}

    Vec& vector() {
        return vector_;
    }

    Tree& tree() {
        return tree_;
    }

    const Vec& vector() const {
        return vector_;
    }

    const Tree& tree() const {
        return tree_;
    }

    void operator=(ThisType&& other)
    {
        Base::operator=(std::move(other));

        tree_  	= std::move(other.tree_);
        vector_ = std::move(other.vector_);

    }

    void operator=(const ThisType& other)
    {
        Base::operator=(other);

        tree_   = other.tree_;
        vector_ = other.vector_;

    }

    void initCtr(Int command)
    {
    	auto& self = this->self();

        tree_.initCtr(&self.allocator(), self.name(), command);
        vector_. initCtr(&tree_, 0, command);

        Base::setCtrShared(NULL);
    }

    void initCtr(const ID& root_id)
    {
    	auto& self = this->self();

    	tree_.initCtr(&self.allocator(), root_id);
        vector_.initCtr(&tree_, get_ctr_root(self.allocator(), root_id, 0));

        Base::setCtrShared(NULL);
    }


    static Int initMetadata()
    {
        Int hash = Tree::initMetadata() + Vec::initMetadata();

        if (Base::getMetadata() == NULL)
        {
            MetadataList list;

            Tree::getMetadata()->putAll(list);
            Vec::getMetadata()->putAll(list);

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
        bool seq_errors     = vector_.check(ptr);

        return tree_errors || seq_errors;
    }

    const CtrShared* shared() const {
        return tree_.shared();
    }

    CtrShared* shared() {
        return tree_.shared();
    }

private:

    static ID get_ctr_root(Allocator& allocator, const ID& root_id, BigInt name)
    {
        typedef typename Tree::NodeBaseG   NodeBaseG;
        typedef typename Tree::Metadata    Metadata;

        NodeBaseG root  = allocator.getPage(root_id, Allocator::READ);
        Metadata  meta  = Tree::getCtrRootMetadata(root);

        return meta.roots(name);
    }

MEMORIA_BT_MODEL_BASE_CLASS_END


}}

#endif
