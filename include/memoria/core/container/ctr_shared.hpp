
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_CONTAINER_CTR_SHRED_HPP
#define	_MEMORIA_CORE_CONTAINER_CTR_SHARED_HPP

#include <vector>

namespace memoria    {

using namespace std;

template <typename ID>
class ContainerShared {
public:
	typedef ContainerShared<ID> CtrShared;
private:

	Int		references_;
	BigInt 	name_;
	ID root_;
	ID root_log_;
	bool updated_;
	bool deleted_;

	vector<CtrShared*>* children_;

	CtrShared* parent_;

public:

	ContainerShared(BigInt name0): references_(0), name_(name0), root_(0), root_log_(0), updated_(false), children_(NULL), parent_(NULL) {}
	ContainerShared(BigInt name0, CtrShared* parent): references_(0), name_(name0), root_(0), root_log_(0), updated_(false), children_(NULL), parent_(parent) {}

	~ContainerShared()
	{
		if (children_ != NULL)
		{
			if (children_->size() > 0)
			{
				cout<<"Delete CtrShared with children at "<<MEMORIA_SOURCE<<endl;
			}

			delete children_;
		}
	}

	Int		references() const {
		return references_;
	}

	Int& 	references() {
		return references_;
	}

	BigInt 	name() const {
		return name_;
	}

	BigInt& name() {
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

	CtrShared* parent() const {
		return parent_;
	}

	CtrShared* parent() {
		return parent_;
	}

	CtrShared* CreateChild(BigInt name)
	{
		CtrShared* shared = new CtrShared(name, this);
		if (this->children_ == NULL)
		{
			this->children_ = new vector<CtrShared*>();
		}

		this->children_->push_back(shared);

		return shared;
	}

	void RemoveChild(CtrShared* shared)
	{
		for (auto i = children_->begin(); i != children_->end(); i++)
		{
			if ((*i) == shared)
			{
				children_->erase(i);
				break;
			}
		}
	}

	CtrShared* Get(BigInt name, bool create = false)
	{
		if (children_ != NULL)
		{
			for (auto i = children_->begin(); i!= children_->end(); i++)
			{
				if ((*i)->name() == name)
				{
					return *i;
				}
			}
		}

		if (create) {
			return CreateChild(name);
		}
		else {
			throw new MemoriaException(MEMORIA_SOURCE, "No child CtrShared is registered for the name", name);
		}
	}

	//FIXME should we ref/unref the parent as well?
	Int ref() {
		return ++references_;
	}

	Int unref() {
		return --references_;
	}
};


}


#endif
