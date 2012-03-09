
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_PROTOTYPES_DYNVECTOR_TOOLS_HPP
#define	_MEMORIA_PROTOTYPES_DYNVECTOR_TOOLS_HPP

#include <memoria/prototypes/btree/tools.hpp>

namespace memoria    	{
namespace btree 		{

using namespace memoria::core;

template <typename NodePage, typename DataPage, Int Size = 16>
class DataPath: public FixedVector<TreePathItem<NodePage>, Size, ValueClearing> {

	typedef FixedVector<TreePathItem<NodePage>, Size, ValueClearing> 	Base;
	typedef DataPath<NodePage, DataPage, Size> 							MyType;

public:

	typedef TreePathItem<DataPage>										DataItem;
private:

	DataItem data_;

public:
	DataPath(): Base() {}

	DataPath(const MyType& other): Base(other)
	{
		data_ = other.data_;
	}

	DataPath(MyType&& other): Base(std::move(other))
	{
		data_ = std::move(other.data_);
	}

	MyType& operator=(const MyType& other)
	{
		Base::operator=(other);

		data_ = other.data_;

		return *this;
	}

	MyType& operator=(MyType&& other)
	{
		Base::operator=(std::move(other));

		return *this;
	}

	DataItem& data()
	{
		return data_;
	}

	const DataItem& data() const
	{
		return data_;
	}
};

}
}


#endif
