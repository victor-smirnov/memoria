
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_CTRWRAPPER_CTR_BASE_HPP
#define _MEMORIA_PROTOTYPES_CTRWRAPPER_CTR_BASE_HPP

#include <memoria/core/container/container.hpp>
#include <memoria/core/container/names.hpp>
#include <memoria/prototypes/btree/macros.hpp>
#include <memoria/core/types/algo.hpp>
#include <memoria/core/tools/fixed_vector.hpp>

#include <memoria/containers/vector_map/names.hpp>

#include <iostream>

namespace memoria       {
namespace vector_map    {

MEMORIA_BTREE_MODEL_BASE_CLASS_NO_CTOR_BEGIN(CtrWrapperContainerBase)

    typedef TypesType                                                           Types;

	typedef typename Types::WrappedCtrName                                      WrappedCtrName;

    typedef typename Types::Profile                                             Profile;
    typedef typename Types::Key                                                 Key;
    typedef typename Types::Value                                               Value;

    typedef typename Base::ID                                                   ID;
    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::CtrShared                                            CtrShared;

    typedef typename CtrTF<Profile, WrappedCtrName, WrappedCtrName>::Type       WrappedCtr;


private:
    WrappedCtr ctr_;


public:

    CtrWrapperContainerBase(): Base(), ctr_(NoParamCtr()) {}

    CtrWrapperContainerBase(const ThisType& other, Allocator* allocator):
        Base(other, allocator),
        ctr_(other.ctr_, allocator)
    {}

    CtrWrapperContainerBase(ThisType&& other, Allocator* allocator):
        Base(std::move(other), allocator),
        ctr_(std::move(other.ctr_), allocator)
    {}

    //broken constructor
    CtrWrapperContainerBase(const ThisType& other):
        Base(other),
        ctr_(NoParamCtr())
    {}

    CtrWrapperContainerBase(ThisType&& other):
        Base(std::move(other)),
        ctr_(NoParamCtr())
    {}

    WrappedCtr& ctr() {
        return ctr_;
    }

    const WrappedCtr& ctr() const {
    	return ctr_;
    }

    void operator=(ThisType&& other)
    {
        Base::operator=(std::move(other));

        ctr_    = std::move(other.ctr_);
    }

    void operator=(const ThisType& other)
    {
        Base::operator=(other);

        ctr_ = other.ctr_;
    }

    void initCtr(Int command)
    {
        ctr_.initCtr(&me()->allocator(), me()->name(), command);

        Base::setCtrShared(NULL);
    }

    void initCtr(const ID& root_id)
    {
        ctr_.initCtr(&me()->allocator(), root_id);

        Base::setCtrShared(NULL);
    }


    static Int initMetadata()
    {
        Int hash = WrappedCtr::initMetadata();

        if (Base::getMetadata() == NULL)
        {
            MetadataList list;

            IdxSet::getMetadata()->putAll(list);
            ByteArray::getMetadata()->putAll(list);

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
        typename IdxSet::Metadata set_meta = set_.getRootMetadata();
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
