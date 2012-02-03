
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

public:

	VectorTest(): SPTestTask(new VectorParams()) {} //

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
			Remove(allocator, dv, params);
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

				if (!Remove(allocator, dv, &params))
				{
					break;
				}

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

				BigInt len = array.Size();
				if (len > 100) len = 100;

				ArrayData prefix(len);
				iter.Skip(-len);
				iter.Read(prefix);

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

				iter.Read(prefix);
				iter.Read(postfix);

				iter.Skip(-postfix.size());

				array.debug() = true;

				iter.Insert(data);

				Check(allocator, "Insertion at the middle of the array failed. See the dump for details.", 	MEMORIA_SOURCE);

				iter.Skip(- data.size() - prefix_len);


				CheckBufferWritten(iter, prefix, 	"Failed to read and compare buffer prefix from array", 	MEMORIA_SOURCE);
				CheckBufferWritten(iter, data, 		"Failed to read and compare buffer from array", 		MEMORIA_SOURCE);
				CheckBufferWritten(iter, postfix, 	"Failed to read and compare buffer postfix from array", MEMORIA_SOURCE);
			}
		}
	}

	bool Remove(Allocator& allocator, ByteVectorCtr& array, VectorReplay* params)
	{
		Int step = params->step_;

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

				BigInt len = array.Size() - size;
				if (len > 100) len = 100;

				ArrayData postfix(len);
				iter.Skip(size);
				iter.Read(postfix);
				iter.Skip(-len - size);

				iter.Remove(size);

				Check(allocator, "Removing region at the start of the array failed. See the dump for details.", MEMORIA_SOURCE);

				CheckBufferWritten(iter, postfix, "Failed to read and compare buffer postfix from array", 		MEMORIA_SOURCE);
			}
			else if (step == 1)
			{
				//Remove at the end of the array
				auto iter = array.Seek(array.Size() - size);

				BigInt len = iter.pos();
				if (len > 100) len = 100;

				ArrayData prefix(len);
				iter.Skip(-len);
				iter.Read(prefix);

				iter.Remove(size);

				Check(allocator, "Removing region at the end of the array failed. See the dump for details.", 	MEMORIA_SOURCE);

				iter.Skip(-len);

				CheckBufferWritten(iter, prefix, "Failed to read and compare buffer prefix from array", 		MEMORIA_SOURCE);
			}
			else {
				//Remove at the middle of the array

				Int pos = GetRandom(array.Size() - size);
				auto iter = array.Seek(pos);

				if (GetRandom(2) == 0)
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
