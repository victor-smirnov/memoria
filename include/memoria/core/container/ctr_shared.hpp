
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_CONTAINER_CTR_SHRED_HPP
#define	_MEMORIA_CORE_CONTAINER_CTR_SHARED_HPP

#include <memoria/core/tools/fixed_vector.hpp>

namespace memoria    {

using namespace memoria::core;
using namespace std;

template <typename ID>
class ContainerShared {
public:

	typedef ContainerShared<ID> 	CtrShared;
private:

	Int		references_;
	BigInt 	name_;
	ID root_;
	ID root_log_;
	bool updated_;
	bool deleted_;

	FixedVector<CtrShared*, 4, NullPtrFunctor> children_;

	CtrShared* parent_;

public:

	ContainerShared(BigInt name0): references_(0), name_(name0), root_(0), root_log_(0), updated_(false), children_(), parent_(NULL) {}
	ContainerShared(BigInt name0, CtrShared* parent): references_(0), name_(name0), root_(0), root_log_(0), updated_(false), children_(), parent_(parent) {}

	//FIXME virtual destructor
	~ContainerShared() throw ()
	{
		for (Int c = 0; c < children_.getSize(); c++)
		{
			if (children_[c] != NULL)
			{
				cout<<"Child CtrShared is not null for name="<<c<<endl;
			}
		}
	}

	Int	references() const
	{
		return references_;
	}

	Int& references()
	{
		return references_;
	}

	BigInt name() const
	{
		return name_;
	}

	BigInt& name()
	{
		return name_;
	}

	const ID& root() const {
		return root_;
	}

	ID& root() {
		return root_;
	}

	const ID& root_log() const {
		return root_log_;
	}

	ID& root_log() {
		return root_log_;
	}

	bool updated() const {
		return updated_;
	}

	bool& updated() {
		return updated_;
	}

	CtrShared* parent() const
	{
		return parent_;
	}

	CtrShared* parent()
	{
		return parent_;
	}

	void RegisterChild(CtrShared* child)
	{
		if (children_.getSize() < child->name())
		{
			for (Int c = children_.getSize(); c < child->name(); c++)
			{
				children_.append(NULL);
			}
		}

		children_[child->name()] = child;
	}

	void UnregisterChild(CtrShared* shared)
	{
		children_[shared->name()] = NULL;
	}

	bool IsChildRegistered(BigInt name)
	{
		if (name < children_.getSize())
		{
			return children_[name] != NULL;
		}

		return false;
	}

	CtrShared* get(BigInt name)
	{
		CtrShared* child = NULL;

		if (name < children_.getSize())
		{
			child = children_[name];
		}

		if (child != NULL)
		{
			return child;
		}
		else {
			throw new Exception(MEMORIA_SOURCE, SBuf()<<"No child CtrShared is registered for the name: "<<name);
		}
	}

	//FIXME should we ref/unref the parent as well?
	Int ref() {
		return ++references_;
	}

	Int unref() {
		return --references_;
	}

	template <typename Allocator>
	void* operator new (size_t size, Allocator* allocator)
	{
		return allocator->allocateMemory(size);
	}

	template <typename Allocator>
	void operator delete (void *ptr, Allocator* allocator)
	{
		allocator->freeMemory(ptr);
	}

	virtual void commit()
	{
		if (updated())
		{
			root() 		= root_log();
			root_log() 	= 0;
			updated() 	= false;
		}

		for (Int c = 0; c < children_.getSize(); c++)
		{
			if (children_[c] != NULL)
			{
				children_[c]->commit();
			}
		}
	}

	virtual void rollback()
	{
		if (updated())
		{
			root_log() 	= 0;
			updated() 	= false;
		}

		for (Int c = 0; c < children_.getSize(); c++)
		{
			if (children_[c] != NULL)
			{
				children_[c]->rollback();
			}
		}
	}
};


}


#endif
