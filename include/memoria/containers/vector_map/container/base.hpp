
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_COMPOSITE_CTR_BASE_HPP
#define	_MEMORIA_PROTOTYPES_COMPOSITE_CTR_BASE_HPP

#include <memoria/core/container/container.hpp>
#include <memoria/core/container/names.hpp>
#include <memoria/prototypes/btree/macros.hpp>
#include <memoria/core/types/algo.hpp>
#include <memoria/core/tools/fixed_vector.hpp>

#include <memoria/containers/vector_map/names.hpp>

#include <iostream>

namespace memoria    	{
namespace vector_map	{

MEMORIA_BTREE_MODEL_BASE_CLASS_NO_CTOR_BEGIN(VectorMapContainerBase)

    typedef TypesType                                                			Types;
	typedef typename Types::Profile                                             Profile;

	typedef typename Base::ID													ID;
	typedef typename Base::Allocator											Allocator;
	typedef typename Base::CtrShared											CtrShared;

	typedef typename CtrTF<Profile, VMSet<2>, VMSet<2> >::Type  				IdxSet;
	typedef typename CtrTF<Profile, Vector,	 Vector>::Type						ByteArray;

	typedef typename IdxSet::Accumulator                        				IdxSetAccumulator;

	typedef typename IdxSet::Key												Key;
	typedef typename IdxSet::Value												ISValue;

	static const Int IS_Indexes													= IdxSet::Indexes;
	static const Int BA_Indexes													= ByteArray::Indexes;

private:
	ByteArray	array_;
	IdxSet 		set_;

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

	IdxSet& set() {
		return set_;
	}

	ByteArray& array() {
		return array_;
	}

	const IdxSet& set() const {
		return set_;
	}

	const ByteArray& array() const {
		return array_;
	}

    void operator=(ThisType&& other)
    {
    	Base::operator=(std::move(other));

    	set_ 	= std::move(other.set_);
    	array_	= std::move(other.array_);
    }

    void operator=(const ThisType& other)
    {
    	Base::operator=(other);

    	set_ 	= other.set_;
    	array_	= other.array_;
    }

    void InitCtr(bool create)
    {
    	array_.InitCtr(me()->allocator(), me()->name(), create);
    	set_.  InitCtr(array_, 0, create);

    	Base::SetCtrShared(NULL);
    }

    void InitCtr(const ID& root_id)
    {
    	array_.InitCtr(me()->allocator(), root_id);
    	set_.InitCtr(array_, get_ctr_root(me()->allocator(), root_id, 0));

    	Base::SetCtrShared(NULL);
    }


    static Int Init()
    {
    	Int hash = IdxSet::Init() + ByteArray::Init();

    	if (Base::reflection() == NULL)
    	{
    		MetadataList list;

    		IdxSet::reflection()->PutAll(list);
    		ByteArray::reflection()->PutAll(list);

    		Base::SetMetadata(new ContainerMetadata("memoria::VectorMap", list, VectorMap::Code, Base::GetContainerInterface()));
    	}

    	return hash;
    }

    virtual ID GetRootID(BigInt name)
    {
    	return me()->array().GetRootID(name);
    }




    virtual void SetRoot(BigInt name, const ID& root_id)
    {
    	me()->array().SetRoot(name, root_id);
    }

    bool Check(void* ptr = NULL)
    {
    	bool array_errors 	= array_.Check(ptr);
    	bool set_errors 	= set_.Check(ptr);

    	return array_errors || set_errors;
    }

    const CtrShared* shared() const {
    	return array_.shared();
    }

    CtrShared* shared() {
    	return array_.shared();
    }


    void SetBranchingFactor(Int count)
    {
    	typename IdxSet::Metadata set_meta = set_.GetRootMetadata();
    	set_meta.branching_factor() = count;
    	set_.SetRootMetadata(set_meta);

    	typename ByteArray::Metadata array_meta = array_.GetRootMetadata();
    	array_meta.branching_factor() = count;
    	array_.SetRootMetadata(array_meta);
    }

    Int GetBranchingFactor() const
    {
    	return set_.GetRootMetadata().branching_factor();
    }

private:

    static ID get_ctr_root(Allocator& allocator, const ID& root_id, BigInt name)
    {
    	typedef typename ByteArray::NodeBaseG 	NodeBaseG;
    	typedef typename ByteArray::Metadata 	Metadata;

    	NodeBaseG root 	= allocator.GetPage(root_id, Allocator::READ);
    	Metadata  meta 	= ByteArray::GetCtrRootMetadata(root);

    	return meta.roots(name);
    }

MEMORIA_BTREE_MODEL_BASE_CLASS_END


}}

#endif
