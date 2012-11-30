
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_COMPOSITE_CTR_BASE_HPP
#define _MEMORIA_PROTOTYPES_COMPOSITE_CTR_BASE_HPP

#include <memoria/core/container/container.hpp>
#include <memoria/core/container/names.hpp>
#include <memoria/prototypes/btree/macros.hpp>
#include <memoria/core/types/algo.hpp>
#include <memoria/core/tools/fixed_vector.hpp>

#include <memoria/containers/vector_map/names.hpp>

#include <iostream>

namespace memoria       {
namespace vector_map    {

MEMORIA_BTREE_MODEL_BASE_CLASS_NO_CTOR_BEGIN(VectorMapContainerBase)

    typedef TypesType                                                           Types;
    typedef typename Types::Profile                                             Profile;

    typedef typename Base::ID                                                   ID;
    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::CtrShared                                            CtrShared;

    typedef typename CtrTF<Profile, VMset<2>, VMset<2> >::Type                  Idxset;
    typedef typename CtrTF<Profile, VectorCtr,  VectorCtr>::Type                ByteArray;

    typedef typename Idxset::Accumulator                                        IdxsetAccumulator;

    typedef typename Idxset::Key                                                Key;
    typedef typename Idxset::Value                                              ISValue;

    static const Int IS_Indexes                                                 = Idxset::Indexes;
    static const Int BA_Indexes                                                 = ByteArray::Indexes;

private:
    ByteArray   array_;
    Idxset      set_;

public:

    VectorMapContainerBase(): Base(), array_(NoParamCtr()), set_(NoParamCtr()) {}

    VectorMapContainerBase(const ThisType& other, Allocator& allocator):
        Base(other, allocator),
        array_(other.array_, allocator),
        set_(other.set_, array_)
    {}

    VectorMapContainerBase(ThisType&& other, Allocator& allocator):
        Base(std::move(other), allocator),
        array_(std::move(other.array_), allocator),
        set_(std::move(other.set_), array_)
    {}

    //broken constructor
    VectorMapContainerBase(const ThisType& other):
        Base(other),
        array_(NoParamCtr()),
        set_(NoParamCtr())
    {}

    VectorMapContainerBase(ThisType&& other):
        Base(std::move(other)),
        array_(NoParamCtr()),
        set_(NoParamCtr())
    {}

    Idxset& set() {
        return set_;
    }

    ByteArray& array() {
        return array_;
    }

    const Idxset& set() const {
        return set_;
    }

    const ByteArray& array() const {
        return array_;
    }

    void operator=(ThisType&& other)
    {
        Base::operator=(std::move(other));

        set_    = std::move(other.set_);
        array_  = std::move(other.array_);
    }

    void operator=(const ThisType& other)
    {
        Base::operator=(other);

        set_    = other.set_;
        array_  = other.array_;
    }

    void initCtr(bool create)
    {
        array_.initCtr(&me()->allocator(), me()->name(), create);
        set_.  initCtr(&array_, 0, create);

        Base::setCtrShared(NULL);
    }

    void initCtr(const ID& root_id)
    {
        array_.initCtr(&me()->allocator(), root_id);
        set_.initCtr(&array_, get_ctr_root(me()->allocator(), root_id, 0));

        Base::setCtrShared(NULL);
    }


    static Int initMetadata()
    {
        Int hash = Idxset::initMetadata() + ByteArray::initMetadata();

        if (Base::reflection() == NULL)
        {
            MetadataList list;

            Idxset::reflection()->putAll(list);
            ByteArray::reflection()->putAll(list);

            Base::setMetadata(new ContainerMetadata(
                                    "memoria::VectorMap",
                                    list,
                                    VectorMapCtr::Code,
                                    Base::getContainerInterface()
                                  )
            );
        }

        return hash;
    }

    virtual ID getRootID(BigInt name)
    {
        return me()->array().getRootID(name);
    }




    virtual void setRoot(BigInt name, const ID& root_id)
    {
        me()->array().setRoot(name, root_id);
    }

    bool check(void* ptr = NULL)
    {
        bool array_errors   = array_.check(ptr);
        bool set_errors     = set_.check(ptr);

        return array_errors || set_errors;
    }

    const CtrShared* shared() const {
        return array_.shared();
    }

    CtrShared* shared() {
        return array_.shared();
    }


    void setBranchingFactor(Int count)
    {
        typename Idxset::Metadata set_meta = set_.getRootMetadata();
        set_meta.branching_factor() = count;
        set_.setRootMetadata(set_meta);

        typename ByteArray::Metadata array_meta = array_.getRootMetadata();
        array_meta.branching_factor() = count;
        array_.setRootMetadata(array_meta);
    }

    Int getBranchingFactor() const
    {
        return set_.getRootMetadata().branching_factor();
    }

private:

    static ID get_ctr_root(Allocator& allocator, const ID& root_id, BigInt name)
    {
        typedef typename ByteArray::NodeBaseG   NodeBaseG;
        typedef typename ByteArray::Metadata    Metadata;

        NodeBaseG root  = allocator.getPage(root_id, Allocator::READ);
        Metadata  meta  = ByteArray::getCtrRootMetadata(root);

        return meta.roots(name);
    }

MEMORIA_BTREE_MODEL_BASE_CLASS_END


}}

#endif
