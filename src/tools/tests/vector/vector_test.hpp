
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_VECTOR_VECTOR_TEST_HPP_
#define MEMORIA_TESTS_VECTOR_VECTOR_TEST_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>

#include "../shared/btree_test_base.hpp"

namespace memoria {

using namespace memoria::vapi;


class VectorReplay: public ReplayParams {
public:
	Int 	data_;
	bool 	insert_;
	Int		block_size_;

	Int		page_step_;

	BigInt 	pos_;

	Int 	cnt_;

	BigInt 	ctr_name_;

public:
	VectorReplay(): ReplayParams(), data_(0), insert_(true), block_size_(0), page_step_(-1), pos_(-1), cnt_(0)
	{
		Add("data", 		data_);
		Add("insert", 		insert_);
		Add("data_size", 	block_size_);
		Add("page_step", 	page_step_);
		Add("pos", 			pos_);
		Add("cnt", 			cnt_);
		Add("ctr_name", 	ctr_name_);
	}
};



class VectorTest: public BTreeBatchTestBase<
	Vector,
	ArrayData,
	VectorReplay
>
{
	typedef VectorTest MyType;
	typedef MyType ParamType;


	typedef BTreeBatchTestBase<
			Vector,
			ArrayData,
			VectorReplay
	>																Base;

	typedef typename Base::Ctr 										Ctr;


	Int 	element_size_;

public:
	VectorTest():
		Base("Vector"), element_size_(1)
	{
		Ctr::Init();

		max_block_size_ = 1024*40;
		size_ 			= 1024*1024*16;

		Add("element_size", element_size_);
	}

	virtual ArrayData CreateBuffer(Ctr& array, Int size, UByte value)
	{
		ArrayData data(size * array.GetElementSize());

		Int esize = array.GetElementSize();

		for (Int c = 0; c < size; c++)
		{
			*(data.data() + c * esize) = value;

			for (Int d = 1; d < esize; d++)
			{
				*(data.data() + c * esize + d) = value == 1 ? 0 : 1;
			}
		}

		return data;
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

	virtual BigInt GetLocalPosition(Iterator& iter)
	{
		return iter.data_pos() / iter.GetElementSize();
	}

	virtual BigInt GetSize(Ctr& array)
	{
		return array.Size();
	}

	virtual void SetElementSize(Ctr& array, ParamType* task_params)
	{
		array.SetElementSize(task_params->element_size_);
	};

	virtual Int GetElementSize(Ctr& array) {
		return array.GetElementSize();
	}

	void CheckIterator(ostream& out, Iterator& iter, const char* source)
	{
		Base::CheckIterator(out, iter, source);

		auto& path = iter.path();

		if (path.data().node().is_set())
		{
			if (iter.data_pos() < 0)
			{
				throw TestException(source, SBuf()<<"iter.data_pos() is negative: "<<iter.data_pos());
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
						throw TestException(source, SBuf()<<"Invalid parent-child relationship for node:"<<path[0]->id()<<" DATA: "<<path.data()->id()<<" idx="<<idx<<" parent_idx="<<path.data().parent_idx());
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
				throw TestException(source, SBuf()<<"Data: "<<path.data()->id()<<" is not fount is it's parent, parent_idx="<<path.data().parent_idx());
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
