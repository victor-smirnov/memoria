
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_VECTOR_TASK_HPP_
#define MEMORIA_TESTS_VECTOR_TASK_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>

#include "../shared/btree_test_base.hpp"

#include "params.hpp"

namespace memoria {

using namespace memoria::vapi;


class VectorTest: public BTreeBatchTestBase<
	Vector,
	ArrayData,
	VectorParams,
	VectorReplay
>
{
	typedef BTreeBatchTestBase<
			Vector,
			ArrayData,
			VectorParams,
			VectorReplay
	>																Base;

	typedef typename Base::Ctr 										Ctr;

public:
	VectorTest(): Base() {
		SmallCtrTypeFactory::Factory<Root>::Type::Init();
		Ctr::Init();
	}

	virtual ArrayData CreateBuffer(Int size, UByte value)
	{
		return memoria::CreateBuffer(size, value);
	}

	virtual Iterator Seek(Ctr& array, BigInt pos)
	{
		return array.Seek(pos);
	}

	virtual void Insert(Iterator& iter, const ArrayData& data)
	{
		iter.Insert(data);
	}

	virtual void Read(Iterator& iter, ArrayData& data)
	{
		iter.Read(data);
	}

	virtual void Remove(Iterator& iter, BigInt size) {
		iter.Remove(size);
	}

	virtual void Skip(Iterator& iter, BigInt offset)
	{
		iter.Skip(offset);
	}

	virtual BigInt GetPosition(Iterator& iter)
	{
		return iter.pos();
	}

	virtual BigInt GetSize(Ctr& array)
	{
		return array.Size();
	}

	void CheckIterator(ostream& out, Iterator& iter, const char* source)
	{
		Base::CheckIterator(out, iter, source);

		auto& path = iter.path();

		if (path.data().node().is_set())
		{
			if (iter.data_pos() < 0)
			{
				throw TestException(source, "iter.data_pos() is negative: "+ToString(iter.data_pos()));
			}

			bool found = false;
			for (Int idx = 0; idx < path[0]->children_count(); idx++)
			{
				ID id = iter.model().GetLeafData(path[0].node(), idx);
				if (id == path.data()->id())
				{
					if (path.data().parent_idx() != idx)
					{
						iter.Dump(out);
						throw TestException(source, "Invalid parent-child relationship for node:"+ToString(IDValue(path[0]->id()))+" DATA: "+ToString(IDValue(path.data()->id()))+" idx="+ToString(idx)+" parent_idx="+ToString(path.data().parent_idx()));
					}
					else {
						found = true;
						break;
					}
				}
			}

			if (!found)
			{
				iter.Dump(out);
				throw TestException(source, "Data: " + ToString(IDValue(path.data()->id())) + " is not fount is it's parent, parent_idx="+ToString(path.data().parent_idx()));
			}
		}


		if (iter.IsEnd())
		{
			if (iter.data().is_set())
			{
				iter.Dump(out);
				throw TestException(MEMORIA_SOURCE, "Iterator is at End but data() is set");
			}
		}
		else {
			if (iter.data().is_empty())
			{
				iter.Dump(out);
				throw TestException(MEMORIA_SOURCE, "Iterator is NOT at End but data() is NOT set");
			}

			if (iter.path().data().parent_idx() != iter.key_idx())
			{
				iter.Dump(out);
				throw TestException(MEMORIA_SOURCE, "Iterator data.parent_idx mismatch");
			}
		}
	}
};



}


#endif
