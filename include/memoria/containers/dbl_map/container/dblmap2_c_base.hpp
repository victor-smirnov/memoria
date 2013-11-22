
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CONTAINERS_DBLMAP2_C_BASE_HPP
#define MEMORIA_CONTAINERS_DBLMAP2_C_BASE_HPP



#include <memoria/core/container/container.hpp>
#include <memoria/core/container/names.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>
#include <memoria/core/types/algo.hpp>
#include <memoria/core/tools/static_array.hpp>

#include <memoria/containers/dbl_map/dblmap_names.hpp>

#include <iostream>

namespace memoria       {
namespace dblmap        {

MEMORIA_BT_MODEL_BASE_CLASS_NO_CTOR_BEGIN(DblMap2CtrBase)

    typedef TypesType                                                           Types;
    typedef typename Types::Profile                                             Profile;

    typedef typename Base::ID                                                   ID;
    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::CtrShared                                            CtrShared;

    typedef typename Types::OuterMapName										OuterMapName;
    typedef typename Types::InnerMapName										InnerMapName;

    typedef typename CtrTF<Profile, OuterMapName, OuterMapName>::Type           OuterMap;
    typedef typename CtrTF<Profile, InnerMapName, InnerMapName>::Type           InnerMap;

    typedef typename Types::Value                                               Value;
    typedef typename Types::Key                                                	Key;

private:
    OuterMap   outer_map_;
    InnerMap   inner_map_;

public:

    DblMap2CtrBase(const CtrInitData& data):
        Base(data),
        outer_map_(data.owner(Base::CONTAINER_HASH)),
        inner_map_(data.owner(Base::CONTAINER_HASH))
    {}

    DblMap2CtrBase(const ThisType& other, Allocator* allocator):
        Base(other, allocator),
        outer_map_(other.outer_map_, allocator),
        inner_map_(other.inner_map_, allocator)
    {}

    DblMap2CtrBase(ThisType&& other, Allocator* allocator):
        Base(std::move(other), allocator),
        outer_map_(std::move(other.outer_map_), allocator),
        inner_map_(std::move(other.inner_map_), allocator)
    {}

    //broken constructor
    DblMap2CtrBase(const ThisType& other):
        Base(other),
        outer_map_(NoParamCtr()),
        inner_map_(NoParamCtr())
    {}

    DblMap2CtrBase(ThisType&& other):
        Base(std::move(other)),
        outer_map_(NoParamCtr()),
        inner_map_(NoParamCtr())
    {}

    InnerMap& inner_map() {
        return inner_map_;
    }

    OuterMap& outer_map() {
        return outer_map_;
    }

    const InnerMap& inner_map() const {
        return inner_map_;
    }

    const OuterMap& outer_map() const {
        return outer_map_;
    }

    void operator=(ThisType&& other)
    {
        Base::operator=(std::move(other));

        outer_map_   = std::move(other.outer_map_);
        inner_map_ = std::move(other.inner_map_);

    }

    void operator=(const ThisType& other)
    {
        Base::operator=(other);

        outer_map_   = other.outer_map_;
        inner_map_ = other.inner_map_;

    }

    void initCtr(Int command)
    {
        auto& self = this->self();

        outer_map_.initCtr(&self.allocator(), self.master_name(), command);
        inner_map_. initCtr(&outer_map_, 0, command);

        Base::setCtrShared(NULL);
    }

    void initCtr(const ID& root_id)
    {
        auto& self = this->self();

        outer_map_.initCtr(&self.allocator(), root_id);
        inner_map_.initCtr(&outer_map_, get_ctr_root(self.allocator(), root_id, -1, 0));

        Base::setCtrShared(NULL);
    }


    virtual bool hasRoot(BigInt name)
    {
        throw vapi::Exception(MA_SRC, "Allocator::hasRoot(BigInt) method must be properly implements for this container");
    }



    static Int initMetadata()
    {
        Int hash = TypeHash<typename Types::ContainerTypeName>::Value;

        OuterMap::initMetadata();
        InnerMap::initMetadata();

        if (Base::getMetadata() == NULL)
        {
            MetadataList list;

            OuterMap::getMetadata()->putAll(list);
            InnerMap::getMetadata()->putAll(list);

            Base::setMetadata(new ContainerMetadata(
                                    TypeNameFactory<typename Types::ContainerTypeName>::name(),
                                    list,
                                    hash,
                                    Base::getContainerInterface()
                                  )
            );

            MetadataRepository<typename Types::Profile>::registerMetadata(Base::getMetadata());
        }

        return hash;
    }

    virtual ID getRootID(BigInt name)
    {
        return self().outer_map().getRootID(name);
    }




    virtual void setRoot(BigInt name, const ID& root_id)
    {
    	cout<<"MrkMap2: SetRoot: "<<name<<" "<<root_id<<endl;

    	self().outer_map().setRoot(name, root_id);
    }

    bool checkTree()
    {
    	return check();
    }

    bool check(void* ptr = nullptr)
    {
        bool outer_map_errors   = outer_map_.check(ptr);
        bool inner_errors     	= inner_map_.check(ptr);

        return outer_map_errors || inner_errors;
    }

    const CtrShared* shared() const {
        return outer_map_.shared();
    }

    CtrShared* shared() {
        return outer_map_.shared();
    }

    void walkTree(ContainerWalker* walker)
    {
    	walker->beginCompositeCtr(
    			TypeNameFactory<typename Types::ContainerTypeName>::name().c_str(),
    			outer_map_.name()
    	);

    	outer_map_.walkTree(walker);
    	inner_map_.walkTree(walker);

    	walker->endCompositeCtr();
    }

private:

    static ID get_ctr_root(Allocator& allocator, const ID& root_id, BigInt ctr_name, BigInt name)
    {
        typedef typename OuterMap::NodeBaseG   NodeBaseG;
        typedef typename OuterMap::Metadata    Metadata;

        NodeBaseG root  = allocator.getPage(root_id, ctr_name);
        Metadata  meta  = OuterMap::getCtrRootMetadata(root);

        return meta.roots(name);
    }

MEMORIA_BT_MODEL_BASE_CLASS_END


}}

#endif
