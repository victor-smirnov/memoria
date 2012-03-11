
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_VECTOR_TASK_HPP_
#define MEMORIA_TESTS_VECTOR_TASK_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>

#include "params.hpp"

namespace memoria {

using namespace memoria::vapi;

class VectorTest: public SPTestTask {

	typedef StreamContainerTypesCollection::Factory<Vector>::Type 	ByteVectorCtr;
	typedef ByteVectorCtr::Iterator									BVIterator;

	typedef typename ByteVectorCtr::ID								ID;
	typedef typename ByteVectorCtr::Value							Value;

public:

	VectorTest(): SPTestTask(new VectorParams()) {}

	virtual ~VectorTest() throw() {}


	BigInt GetRandomPosition(ByteVectorCtr& array)
	{
		BigInt size = array.Size();
		return GetBIRandom(size);
	}

	virtual TestReplayParams* CreateTestStep(StringRef name) const
	{
		return new VectorReplay();
	}

	virtual void Replay(ostream& out, TestReplayParams* step_params)
	{
		VectorReplay* params = static_cast<VectorReplay*>(step_params);
		Allocator allocator;
		LoadAllocator(allocator, params);
		ByteVectorCtr dv(allocator, 1);

		dv.SetMaxChildrenPerNode(params->btree_airity_);

		if (params->insert_)
		{
			Build(out, allocator, dv, params);
		}
		else {
			Remove(out, allocator, dv, params);
		}
	}

	virtual void Run(ostream& out)
	{
		VectorReplay params;
		VectorParams* task_params = GetParameters<VectorParams>();

		if (task_params->btree_random_airity_)
		{
			task_params->btree_airity_ = 8 + GetRandom(100);
			out<<"BTree Airity: "<<task_params->btree_airity_<<endl;
		}

		params.size_			= task_params->size_;
		params.btree_airity_ 	= task_params->btree_airity_;

		for (Int step = 0; step < 3; step++)
		{
			params.step_ = step;
			Run(out, params, task_params, false);
		}

		// Run() will use different step for each ByteArray update operation
		Run(out, params, task_params, true);

//		params.step_ = 2;
//		Run(out, params, task_params, false);
	}


	void Run(ostream& out, VectorReplay& params, VectorParams* task_params, bool step)
	{
		DefaultLogHandlerImpl logHandler(out);

		Allocator allocator;
		allocator.GetLogger()->SetHandler(&logHandler);
		ByteVectorCtr dv(allocator, 1, true);

		dv.SetMaxChildrenPerNode(params.btree_airity_);

		try {
			out<<"Insert data"<<endl;
			params.insert_ = true;

			params.data_ = 1;
			while (dv.Size() < params.size_)
			{
				if (step)
				{
					params.step_ 		= GetRandom(3);
				}

				params.data_size_ 	= 1 + GetRandom(task_params->max_block_size_);

				Build(out, allocator, dv, &params);
				allocator.commit();
				params.data_++;

				params.pos_ 		= -1;
				params.page_step_ 	= -1;
				params.cnt_++;
			}

			//StoreAllocator(allocator, "allocator.dump");

			out<<"Remove data. ByteVector contains "<<(dv.Size()/1024)<<"K bytes"<<endl;
			params.insert_ = false;

			for (Int c = 0; ; c++)
			{
				if (step)
				{
					params.step_ = GetRandom(3);
				}

				BigInt size = dv.Size();
				BigInt max_size = task_params->max_block_size_ <= size ? task_params->max_block_size_ : size;

				params.data_size_ = 1 + GetBIRandom(max_size);
				params.page_step_ 	= GetRandom(3);

				if (!Remove(out, allocator, dv, &params))
				{
					break;
				}

				params.pos_ 		= -1;
				params.page_step_ 	= -1;

				allocator.commit();
			}

			out<<"Vector.size = "<<(dv.Size() / 1024)<<"K bytes"<<endl;

			allocator.commit();
		}
		catch (...)
		{
			Store(allocator, &params);
			throw;
		}
	}

	void CheckIterator(ostream& out, BVIterator& iter, const char* source)
	{
		auto& path = iter.path();
		for (Int level = path.GetSize() - 1; level > 0; level--)
		{
			for (Int idx = 0; idx < path[level]->children_count(); idx++)
			{
				ID id = iter.model().GetINodeData(path[level].node(), idx);
				if (id == path[level - 1]->id() && path[level - 1].parent_idx() != idx)
				{
					iter.Dump(out);
					throw TestException(source, "Invalid parent-child relationship for node:" + ToString(IDValue(path[level]->id())) + " child: "+ToString(IDValue(path[level - 1]->id()))+" idx="+ToString(idx)+" parent_idx="+ToString(path[level-1].parent_idx()));
				}
			}
		}

		if (path.data().node().is_set())
		{
			if (iter.data_pos() < 0)
			{
				throw TestException(source, "iter.data_pos() is negative: "+ToString(iter.data_pos()));
			}

			for (Int idx = 0; idx < path[0]->children_count(); idx++)
			{
				ID id = iter.model().GetLeafData(path[0].node(), idx);
				if (id == path.data()->id() && path.data().parent_idx() != idx)
				{
					iter.Dump(out);
					throw TestException(source, "Invalid parent-child relationship for node:"+ToString(IDValue(path[0]->id()))+" DATA: "+ToString(IDValue(path.data()->id()))+" idx="+ToString(idx)+" parent_idx="+ToString(path.data().parent_idx()));
				}
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
				throw TestException(MEMORIA_SOURCE, "Iterator is NOT at End but data() is NOT set");
			}
		}
	}

	void Build(ostream& out, Allocator& allocator, ByteVectorCtr& array, VectorReplay *params)
	{
		UByte value = params->data_;
		Int step 	= params->step_;

		ArrayData data = CreateBuffer(params->data_size_, value);

		BigInt size = array.Size();

		if (size == 0)
		{
			//Insert buffer into an empty array
			auto iter = array.Seek(0);

			iter.Insert(data);

			Check(allocator, "Insertion into an empty array failed. See the dump for details.", MEMORIA_SOURCE);

			auto iter1 = array.Seek(0);
			CheckBufferWritten(iter1, data, "Failed to read and compare buffer from array", MEMORIA_SOURCE);
		}
		else {
			if (step == 0)
			{
				//Insert at the start of the array
				auto iter = array.Seek(0);

				BigInt len = array.Size();
				if (len > 100) len = 100;

				ArrayData postfix(len);
				iter.Read(postfix);

				iter.Skip(-len);

				iter.Insert(data);

				Check(allocator, "Insertion at the start of the array failed. See the dump for details.", 	MEMORIA_SOURCE);

				iter.Skip(-data.size());

				CheckBufferWritten(iter, data, "Failed to read and compare buffer from array", 				MEMORIA_SOURCE);

				CheckBufferWritten(iter, postfix, "Failed to read and compare buffer postfix from array", 	MEMORIA_SOURCE);
			}
			else if (step == 1)
			{
				//Insert at the end of the array
				auto iter = array.Seek(array.Size());

				CheckIterator(out, iter, MEMORIA_SOURCE);

				BigInt len = array.Size();
				if (len > 100) len = 100;

				ArrayData prefix(len);
				iter.Skip(-len);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				iter.Read(prefix);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				iter.Insert(data);

				Check(allocator, "Insertion at the end of the array failed. See the dump for details.", MEMORIA_SOURCE);

				iter.Skip(-data.size() - len);

				CheckBufferWritten(iter, prefix, "Failed to read and compare buffer prefix from array", MEMORIA_SOURCE);
				CheckBufferWritten(iter, data, "Failed to read and compare buffer from array", 			MEMORIA_SOURCE);
			}
			else {
				//Insert at the middle of the array

				if (params->pos_ == -1) params->pos_ = GetRandomPosition(array);

				Int pos = params->pos_;

				auto iter = array.Seek(pos);

				if (params->page_step_ == -1) params->page_step_ = GetRandom(2);

				if (params->page_step_ == 0)
				{
					iter.Skip(-iter.data_pos());
					pos = iter.pos();
				}

				BigInt prefix_len = pos;
				if (prefix_len > 100) prefix_len = 100;

				BigInt postfix_len = array.Size() - pos;
				if (postfix_len > 100) postfix_len = 100;

				ArrayData prefix(prefix_len);
				ArrayData postfix(postfix_len);

				iter.Skip(-prefix_len);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				iter.Read(prefix);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				iter.Read(postfix);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				iter.Skip(-postfix.size());
				CheckIterator(out, iter, MEMORIA_SOURCE);

				iter.Insert(data);

				Check(allocator, "Insertion at the middle of the array failed. See the dump for details.", 	MEMORIA_SOURCE);

				CheckIterator(out, iter, MEMORIA_SOURCE);

				iter.Skip(- data.size() - prefix_len);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				CheckBufferWritten(iter, prefix, 	"Failed to read and compare buffer prefix from array", 	MEMORIA_SOURCE);

				CheckIterator(out, iter, MEMORIA_SOURCE);

				CheckBufferWritten(iter, data, 		"Failed to read and compare buffer from array", 		MEMORIA_SOURCE);
				CheckBufferWritten(iter, postfix, 	"Failed to read and compare buffer postfix from array", MEMORIA_SOURCE);
			}
		}
	}

	bool Remove(ostream& out, Allocator& allocator, ByteVectorCtr& array, VectorReplay* params)
	{
		Int step = params->step_;

		params->cnt_++;

		if (array.Size() < 20000)
		{
			auto iter = array.Seek(0);
			iter.Remove(array.Size());

			Check(allocator, "Remove ByteArray", MEMORIA_SOURCE);
			return array.Size() > 0;
		}
		else {
			BigInt size = params->data_size_;

			if (step == 0)
			{
				//Remove at the start of the array
				auto iter = array.Seek(0);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				BigInt len = array.Size() - size;
				if (len > 100) len = 100;

				ArrayData postfix(len);
				iter.Skip(size);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				iter.Read(postfix);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				iter.Skip(-len - size);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				iter.Remove(size);

				Check(allocator, "Removing region at the start of the array failed. See the dump for details.", MEMORIA_SOURCE);

				CheckIterator(out, iter, MEMORIA_SOURCE);

				CheckBufferWritten(iter, postfix, "Failed to read and compare buffer postfix from array", 		MEMORIA_SOURCE);
			}
			else if (step == 1)
			{
				//Remove at the end of the array
				auto iter = array.Seek(array.Size() - size);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				BigInt len = iter.pos();
				if (len > 100) len = 100;

				ArrayData prefix(len);
				iter.Skip(-len);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				iter.Read(prefix);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				iter.Remove(size);

				Check(allocator, "Removing region at the end of the array failed. See the dump for details.", 	MEMORIA_SOURCE);

				CheckIterator(out, iter, MEMORIA_SOURCE);

				iter.Skip(-len);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				CheckBufferWritten(iter, prefix, "Failed to read and compare buffer prefix from array", 		MEMORIA_SOURCE);
			}
			else {
				//Remove at the middle of the array

				if (params->pos_ == -1) params->pos_ = GetRandomPosition(array);

				Int pos = params->pos_;

				auto iter = array.Seek(pos);

				if (params->page_step_ == -1) params->page_step_ = GetRandom(2);

				if (params->page_step_ == 0)
				{
					iter.Skip(-iter.data_pos());
					pos = iter.pos();
				}

				BigInt prefix_len = pos;
				if (prefix_len > 100) prefix_len = 100;

				BigInt postfix_len = array.Size() - (pos + size);
				if (postfix_len > 100) postfix_len = 100;

				ArrayData prefix(prefix_len);
				ArrayData postfix(postfix_len);

				iter.Skip(-prefix_len);

				iter.Read(prefix);

				iter.Skip(size);

				iter.Read(postfix);

				iter.Skip(-postfix.size() - size);

				iter.Remove(size);

				Check(allocator, "Removing region at the middle of the array failed. See the dump for details.", 	MEMORIA_SOURCE);

				CheckIterator(out, iter, MEMORIA_SOURCE);

				iter.Skip(-prefix_len);

				CheckBufferWritten(iter, prefix, 	"Failed to read and compare buffer prefix from array", 			MEMORIA_SOURCE);
				CheckBufferWritten(iter, postfix, 	"Failed to read and compare buffer postfix from array", 		MEMORIA_SOURCE);
			}

			return array.Size() > 0;
		}
	}

};


}


#endif
