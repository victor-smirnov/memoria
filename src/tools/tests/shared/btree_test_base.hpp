
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_SHARED_BTREE_TEST_BASE_HPP_
#define MEMORIA_TESTS_SHARED_BTREE_TEST_BASE_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>



namespace memoria {

template <
	typename ContainerTypeName,
	typename ArrayData,
	typename ParamType,
	typename ReplayParamType
>
class BTreeBatchTestBase: public SPTestTask {

protected:
	typedef typename SmallCtrTypeFactory::Factory<ContainerTypeName>::Type 	Ctr;
	typedef typename Ctr::Iterator														Iterator;
	typedef typename Ctr::Accumulator													Accumulator;
	typedef typename Ctr::ID															ID;

public:

	BTreeBatchTestBase(): SPTestTask(new ParamType()) {}
	virtual ~BTreeBatchTestBase() throw() {}

	virtual ArrayData CreateBuffer(Int size, UByte value) 				= 0;
	virtual Iterator Seek(Ctr& array, BigInt pos) 						= 0;
	virtual void Insert(Iterator& iter, const ArrayData& data) 			= 0;
	virtual void Read(Iterator& iter, ArrayData& data) 					= 0;
	virtual void Remove(Iterator& iter, BigInt size) 					= 0;
	virtual void Skip(Iterator& iter, BigInt offset) 					= 0;
	virtual BigInt GetPosition(Iterator& iter)							= 0;
	virtual BigInt GetSize(Ctr& array)									= 0;

	virtual ReplayParamType* CreateTestStep(StringRef name) const
	{
		return new ReplayParamType();
	}

	virtual void Replay(ostream& out, TestReplayParams* step_params)
	{
		ReplayParamType* params = static_cast<ReplayParamType*>(step_params);
		Allocator allocator;
		LoadAllocator(allocator, params);

		Check(allocator, "Allocator Check failed", 	MEMORIA_SOURCE);

		Ctr dv(allocator, 1);

		dv.SetMaxChildrenPerNode(params->btree_airity_);

		if (params->insert_)
		{
			Build(out, allocator, dv, params);
		}
		else {
			Remove(out, allocator, dv, params);
		}
	}


	virtual BigInt GetRandomPosition(Ctr& array)
	{
		BigInt size = GetSize(array);
		return GetBIRandom(size);
	}

	virtual void Run(ostream& out)
	{
		ReplayParamType params;
		ParamType* task_params = GetParameters<ParamType>();

		if (task_params->btree_random_airity_)
		{
			task_params->btree_airity_ = 8 + GetRandom(100);
			out<<"BTree Airity: "<<task_params->btree_airity_<<endl;
		}

		params.size_			= task_params->size_;
		params.btree_airity_ 	= task_params->btree_airity_;

		for (Int step = 0; step < 2; step++)
		{
			params.step_ = step;
			Run(out, params, task_params, false);
		}

		// Run() will use different step for each ByteArray update operation
		Run(out, params, task_params, true);
	}




	virtual void Run(ostream& out, ReplayParamType& params, ParamType* task_params, bool step)
	{
		DefaultLogHandlerImpl logHandler(out);

		Allocator allocator;
		allocator.GetLogger()->SetHandler(&logHandler);
		Ctr dv(allocator, 1, true);

		allocator.commit();

		dv.SetMaxChildrenPerNode(params.btree_airity_);

		try {
			out<<"Insert data"<<endl;
			params.insert_ = true;

			params.data_ = 1;
			while (GetSize(dv) < params.size_)
			{
				if (step)
				{
					params.step_ 		= GetRandom(3);
				}

				params.block_size_ 	= 1 + GetRandom(task_params->max_block_size_);

				Build(out, allocator, dv, &params);

				allocator.commit();

				params.data_++;

				params.pos_ 		= -1;
				params.page_step_ 	= -1;
			}


			out<<"Remove data. SumSet contains "<<(GetSize(dv)/1024)<<"K keys"<<endl;
			params.insert_ = false;

			for (Int c = 0; ; c++)
			{
				if (step)
				{
					params.step_ = GetRandom(3);
				}

				BigInt size = GetSize(dv);
				BigInt max_size = task_params->max_block_size_ <= size ? task_params->max_block_size_ : size;

				params.block_size_  = 1 + GetBIRandom(max_size);
				params.page_step_ 	= GetRandom(3);

				if (!Remove(out, allocator, dv, &params))
				{
					break;
				}

				params.pos_ 		= -1;
				params.page_step_ 	= -1;

				allocator.commit();
			}

			out<<"SumSet.size = "<<(GetSize(dv) / 1024)<<"K keys"<<endl;

			allocator.commit();
		}
		catch (...)
		{
			Store(allocator, &params);
			throw;
		}
	}



	void Build(ostream& out, Allocator& allocator, Ctr& array, ReplayParamType *params)
	{
		UByte value = params->data_;
		Int step 	= params->step_;

		ArrayData data = CreateBuffer(params->block_size_, value);

		BigInt size = GetSize(array);

		if (size == 0)
		{
			//Insert buffer into an empty array
			auto iter = Seek(array, 0);
			CheckIterator(out, iter, MEMORIA_SOURCE);

			Insert(iter, data);
			CheckIterator(out, iter, MEMORIA_SOURCE);

			Check(allocator, "Insertion into an empty array failed. See the dump for details.", MEMORIA_SOURCE);

			auto iter1 = Seek(array, 0);
			CheckBufferWritten(iter1, data, "Failed to read and compare buffer from array", MEMORIA_SOURCE);
		}
		else {
			if (step == 0)
			{
				//Insert at the start of the array
				auto iter = Seek(array, 0);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				BigInt len = GetSize(array);
				if (len > 100) len = 100;

				ArrayData postfix(len);

				Read(iter, postfix);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				Skip(iter, -len);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				Insert(iter, data);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				Check(allocator, "Insertion at the start of the array failed. See the dump for details.", 	MEMORIA_SOURCE);

				Skip(iter, -data.size());
				CheckIterator(out, iter, MEMORIA_SOURCE);

				CheckBufferWritten(iter, data, "Failed to read and compare buffer from array", 				MEMORIA_SOURCE);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				CheckBufferWritten(iter, postfix, "Failed to read and compare buffer postfix from array", 	MEMORIA_SOURCE);
				CheckIterator(out, iter, MEMORIA_SOURCE);
			}
			else if (step == 1)
			{
				//Insert at the end of the array
				BigInt len = GetSize(array);

				auto iter = Seek(array, len);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				if (len > 100) len = 100;

				ArrayData prefix(len);
				Skip(iter, -len);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				Read(iter, prefix);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				Insert(iter, data);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				Check(allocator, "Insertion at the end of the array failed. See the dump for details.", MEMORIA_SOURCE);

				Skip(iter, -data.size() - len);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				CheckBufferWritten(iter, prefix, "Failed to read and compare buffer prefix from array", MEMORIA_SOURCE);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				CheckBufferWritten(iter, data, "Failed to read and compare buffer from array", 			MEMORIA_SOURCE);
				CheckIterator(out, iter, MEMORIA_SOURCE);
			}
			else {
				//Insert in the middle of the array

				if (params->pos_ == -1) params->pos_ = GetRandomPosition(array);

				Int pos = params->pos_;

				auto iter = Seek(array, pos);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				if (params->page_step_ == -1) params->page_step_ = GetRandom(2);

				if (params->page_step_ == 0)
				{
					Skip(iter, -iter.key_idx());
					CheckIterator(out, iter, MEMORIA_SOURCE);
					pos = GetPosition(iter);
				}

				BigInt prefix_len = pos;
				if (prefix_len > 100) prefix_len = 100;

				BigInt postfix_len = GetSize(array) - pos;
				if (postfix_len > 100) postfix_len = 100;

				ArrayData prefix(prefix_len);
				ArrayData postfix(postfix_len);

				Skip(iter, -prefix_len);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				Read(iter, prefix);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				Read(iter, postfix);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				Skip(iter, -postfix.size());
				CheckIterator(out, iter, MEMORIA_SOURCE);

				Insert(iter, data);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				Check(allocator, "Insertion at the middle of the array failed. See the dump for details.", 	MEMORIA_SOURCE);

				Skip(iter, - data.size() - prefix_len);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				CheckBufferWritten(iter, prefix, 	"Failed to read and compare buffer prefix from array", 	MEMORIA_SOURCE);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				CheckBufferWritten(iter, data, 		"Failed to read and compare buffer from array", 		MEMORIA_SOURCE);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				CheckBufferWritten(iter, postfix, 	"Failed to read and compare buffer postfix from array", MEMORIA_SOURCE);
				CheckIterator(out, iter, MEMORIA_SOURCE);
			}
		}
	}

	bool Remove(ostream& out, Allocator& allocator, Ctr& array, ReplayParamType* params)
	{
		Int step = params->step_;

		params->cnt_++;

		if (GetSize(array) < 200)
		{
			auto iter = array.Begin();
			CheckIterator(out, iter, MEMORIA_SOURCE);

			Remove(iter, GetSize(array));
			CheckIterator(out, iter, MEMORIA_SOURCE);

			Check(allocator, "Remove ByteArray", MEMORIA_SOURCE);
			return GetSize(array) > 0;
		}
		else {
			BigInt size = params->block_size_;

			if (step == 0)
			{
				//Remove at the start of the array
				auto iter = Seek(array, 0);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				BigInt len = GetSize(array) - size;
				if (len > 100) len = 100;

				ArrayData postfix(len);
				Skip(iter, size);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				Read(iter, postfix);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				Skip(iter, -len - size);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				Remove(iter, size);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				Check(allocator, "Removing region at the start of the array failed. See the dump for details.", MEMORIA_SOURCE);

				CheckBufferWritten(iter, postfix, "Failed to read and compare buffer postfix from array", 		MEMORIA_SOURCE);
				CheckIterator(out, iter, MEMORIA_SOURCE);
			}
			else if (step == 1)
			{
				//Remove at the end of the array
				auto iter = Seek(array, GetSize(array) - size);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				BigInt len = GetPosition(iter);
				if (len > 100) len = 100;

				ArrayData prefix(len);
				Skip(iter, -len);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				Read(iter, prefix);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				Remove(iter, size);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				Check(allocator, "Removing region at the end of the array failed. See the dump for details.", 	MEMORIA_SOURCE);

				Skip(iter, -len);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				CheckBufferWritten(iter, prefix, "Failed to read and compare buffer prefix from array", 		MEMORIA_SOURCE);
				CheckIterator(out, iter, MEMORIA_SOURCE);
			}
			else {
				//Remove at the middle of the array

				if (params->cnt_ == 65) {
					int a = 0; a++;
				}

				if (params->pos_ == -1) params->pos_ = GetRandomPosition(array);

				Int pos = params->pos_;

				auto iter = Seek(array, pos);

				if (params->page_step_ == -1) params->page_step_ = GetRandom(2);

				if (params->page_step_ == 0)
				{
					Skip(iter, -iter.key_idx());
					CheckIterator(out, iter, MEMORIA_SOURCE);

					pos = GetPosition(iter);
				}

				if (pos + size > GetSize(array))
				{
					size = GetSize(array) - pos - 1;
				}

				BigInt prefix_len = pos;
				if (prefix_len > 100) prefix_len = 100;

				BigInt postfix_len = GetSize(array) - (pos + size);
				if (postfix_len > 100) postfix_len = 100;

				ArrayData prefix(prefix_len);
				ArrayData postfix(postfix_len);

				Skip(iter, -prefix_len);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				Read(iter, prefix);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				Skip(iter, size);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				Read(iter, postfix);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				Skip(iter, -postfix.size() - size);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				Remove(iter, size);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				Check(allocator, "Removing region at the middle of the array failed. See the dump for details.", 	MEMORIA_SOURCE);

				Skip(iter, -prefix_len);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				CheckBufferWritten(iter, prefix, 	"Failed to read and compare buffer prefix from array", 			MEMORIA_SOURCE);
				CheckIterator(out, iter, MEMORIA_SOURCE);

				CheckBufferWritten(iter, postfix, 	"Failed to read and compare buffer postfix from array", 		MEMORIA_SOURCE);
				CheckIterator(out, iter, MEMORIA_SOURCE);
			}

			return GetSize(array) > 0;
		}

		return false;
	}


	virtual void CheckIterator(ostream& out, Iterator& iter, const char* source)
	{
		CheckIteratorPrefix(out, iter, source);

		auto& path = iter.path();

		for (Int level = path.GetSize() - 1; level > 0; level--)
		{
			bool found = false;

			for (Int idx = 0; idx < path[level]->children_count(); idx++)
			{
				ID id = iter.model().GetINodeData(path[level].node(), idx);
				if (id == path[level - 1]->id())
				{
					if (path[level - 1].parent_idx() != idx)
					{
						iter.Dump(out);
						throw TestException(source, "Invalid parent-child relationship for node:" + ToString(IDValue(path[level]->id())) + " child: "+ToString(IDValue(path[level - 1]->id()))+" idx="+ToString(idx)+" parent_idx="+ToString(path[level-1].parent_idx()));
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
				throw TestException(source, "Child: " + ToString(IDValue(path[level - 1]->id())) + " is not fount is it's parent, parent_idx="+ToString(path[level - 1].parent_idx()));
			}
		}


	}

	virtual void CheckIteratorPrefix(ostream& out, Iterator& iter, const char* source)
	{
		Accumulator prefix;

		iter.ComputePrefix(prefix);

		if (iter.prefix(0) != prefix.key(0))
		{
			iter.Dump(out);
			throw TestException(source, "Invalid prefix value. Iterator: "+ToString(iter.prefix())+" Actual: "+ToString(prefix));
		}
	}

};

}


#endif
