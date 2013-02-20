
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef MEMORIA_CONTAINERS_LOUDS_CONTAINER_BASE_HPP
#define MEMORIA_CONTAINERS_LOUDS_CONTAINER_BASE_HPP

#include <memoria/core/container/container.hpp>
#include <memoria/core/container/names.hpp>
#include <memoria/prototypes/btree/macros.hpp>
#include <memoria/core/types/algo.hpp>
#include <memoria/core/tools/fixed_vector.hpp>

#include <memoria/containers/louds/louds_names.hpp>

#include <iostream>

namespace memoria       {
namespace vector_map    {

MEMORIA_BTREE_MODEL_BASE_CLASS_NO_CTOR_BEGIN(LoudsContainerBase)

    typedef TypesType                                                           Types;
    typedef typename Types::Profile                                             Profile;
    typedef typename Types::Key                                                 Key;
    typedef typename Types::Value                                               Value;

    typedef typename Base::ID                                                   ID;
    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::CtrShared                                            CtrShared;

    typedef typename CtrTF<Profile, VMSet<Key, 2>, VMSet<Key, 2> >::Type        IdxSet;
    typedef typename CtrTF<Profile, Sequence<1>, Sequence<1> >::Type        	Louds;

    typedef typename IdxSet::Accumulator                                        IdxsetAccumulator;

    typedef typename IdxSet::Value                                              ISValue;

    static const Int IS_Indexes                                                 = IdxSet::Indexes;
    static const Int BA_Indexes                                                 = Louds::Indexes;

private:
    Louds   	louds_;
    IdxSet      set_;

public:

    LoudsContainerBase(): Base(), louds_(NoParamCtr()), set_(NoParamCtr()) {}

    LoudsContainerBase(const ThisType& other, Allocator* allocator):
        Base(other, allocator),
        louds_(other.louds_, allocator),
        set_(other.set_, allocator)
    {}

    LoudsContainerBase(ThisType&& other, Allocator* allocator):
        Base(std::move(other), allocator),
        louds_(std::move(other.louds_), allocator),
        set_(std::move(other.set_), allocator)
    {}

    //broken constructor
    LoudsContainerBase(const ThisType& other):
        Base(other),
        louds_(NoParamCtr()),
        set_(NoParamCtr())
    {}

    LoudsContainerBase(ThisType&& other):
        Base(std::move(other)),
        louds_(NoParamCtr()),
        set_(NoParamCtr())
    {}

    IdxSet& set() {
        return set_;
    }

    Louds& louds() {
        return louds_;
    }

    const IdxSet& set() const {
        return set_;
    }

    const Louds& louds() const {
        return louds_;
    }

    void operator=(ThisType&& other)
    {
        Base::operator=(std::move(other));

        set_    = std::move(other.set_);
        louds_  = std::move(other.louds_);
    }

    void operator=(const ThisType& other)
    {
        Base::operator=(other);

        set_    = other.set_;
        louds_  = other.louds_;
    }

    void initCtr(Int command)
    {
        louds_.initCtr(&me()->allocator(), me()->name(), command);
        set_.  initCtr(&louds_, 0, command);

        Base::setCtrShared(NULL);
    }

    void initCtr(const ID& root_id)
    {
        louds_.initCtr(&me()->allocator(), root_id);
        set_.initCtr(&louds_, get_ctr_root(me()->allocator(), root_id, 0));

        Base::setCtrShared(NULL);
    }


    static Int initMetadata()
    {
        Int hash = IdxSet::initMetadata() + Louds::initMetadata();

        if (Base::getMetadata() == NULL)
        {
            MetadataList list;

            IdxSet::getMetadata()->putAll(list);
            Louds::getMetadata()->putAll(list);

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
        return me()->louds().getRootID(name);
    }




    virtual void setRoot(BigInt name, const ID& root_id)
    {
        me()->louds().setRoot(name, root_id);
    }

    bool check(void* ptr = NULL)
    {
        bool array_errors   = louds_.check(ptr);
        bool set_errors     = set_.check(ptr);

        return array_errors || set_errors;
    }

    const CtrShared* shared() const {
        return louds_.shared();
    }

    CtrShared* shared() {
        return louds_.shared();
    }


    void setBranchingFactor(Int count)
    {
        typename IdxSet::Metadata set_meta = set_.getRootMetadata();
        set_meta.branching_factor() = count;
        set_.setRootMetadata(set_meta);

        typename Louds::Metadata array_meta = louds_.getRootMetadata();
        array_meta.branching_factor() = count;
        louds_.setRootMetadata(array_meta);
    }

    Int getBranchingFactor() const
    {
        return set_.getRootMetadata().branching_factor();
    }

private:

    static ID get_ctr_root(Allocator& allocator, const ID& root_id, BigInt name)
    {
        typedef typename Louds::NodeBaseG   NodeBaseG;
        typedef typename Louds::Metadata    Metadata;

        NodeBaseG root  = allocator.getPage(root_id, Allocator::READ);
        Metadata  meta  = Louds::getCtrRootMetadata(root);

        return meta.roots(name);
    }

MEMORIA_BTREE_MODEL_BASE_CLASS_END


}}

#endif
