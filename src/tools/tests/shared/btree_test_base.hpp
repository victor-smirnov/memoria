
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
	typename ReplayParamType
>
class BTreeBatchTestBase: public SPTestTask {

	typedef BTreeBatchTestBase<ContainerTypeName, ArrayData, ReplayParamType> 			MyType;
	typedef MyType 																		ParamType;

protected:
	typedef typename SmallCtrTypeFactory::Factory<ContainerTypeName>::Type 	Ctr;
	typedef typename Ctr::Iterator														Iterator;
	typedef typename Ctr::Accumulator													Accumulator;
	typedef typename Ctr::ID															ID;

	Int max_block_size_;

public:

	BTreeBatchTestBase(StringRef name):
		SPTestTask(name),
		max_block_size_(1024*40)
	{
		size_ = 1024*1024*16;
		Add("max_block_size", max_block_size_);
	}

	virtual ~BTreeBatchTestBase() throw() {}

	virtual ArrayData CreateBuffer(Ctr& array, Int size, UByte value) 	= 0;
	virtual Iterator seek(Ctr& array, BigInt pos) 						= 0;
	virtual void insert(Iterator& iter, const ArrayData& data) 			= 0;
	virtual void Read(Iterator& iter, ArrayData& data) 					= 0;
	virtual void remove(Iterator& iter, BigInt size) 					= 0;
	virtual void Skip(Iterator& iter, BigInt offset) 					= 0;
	virtual BigInt getPosition(Iterator& iter)							= 0;
	virtual BigInt getLocalPosition(Iterator& iter)						= 0;
	virtual BigInt getSize(Ctr& array)									= 0;

	virtual Int getElementSize(Ctr& array) {
		return 1;
	}

	virtual void setElementSize(Ctr& array, ParamType* task_params) {}

	virtual ReplayParamType* CreateTestStep(StringRef name) const
	{
		return new ReplayParamType();
	}

	virtual void Replay(ostream& out, TestReplayParams* step_params)
	{
		ReplayParamType* params = static_cast<ReplayParamType*>(step_params);
		Allocator allocator;
		LoadAllocator(allocator, params);

		check(allocator, "Allocator check failed", 	MEMORIA_SOURCE);

		Ctr dv(allocator, params->ctr_name_);

		if (params->insert_)
		{
			Build(out, allocator, dv, params);
		}
		else {
			remove(out, allocator, dv, params);
		}
	}


	virtual BigInt getRandomPosition(Ctr& array)
	{
		BigInt size = getSize(array);
		return getBIRandom(size);
	}

	virtual void Run(ostream& out)
	{
		ReplayParamType params;
		ParamType* task_params = getParameters<ParamType>();

		if (task_params->btree_random_branching_)
		{
			task_params->btree_branching_ = 8 + getRandom(100);
			out<<"BTree Branching: "<<task_params->btree_branching_<<endl;
		}

		out<<"Max Block Size: "<<task_params->max_block_size_<<endl;

		params.size_				= task_params->size_;

		for (Int step = 0; step < 2; step++)
		{
			params.step_ = step;
			Run(out, params, task_params, false, task_params->btree_branching_);
		}

		// Run() will use different step for each ByteArray update operation
		Run(out, params, task_params, true, task_params->btree_branching_);
	}




	virtual void Run(ostream& out, ReplayParamType& params, ParamType* task_params, bool step, Int branching)
	{
		DefaultLogHandlerImpl logHandler(out);

		Allocator allocator;
		allocator.getLogger()->setHandler(&logHandler);

		Ctr dv(allocator);

		setElementSize(dv, task_params);

		params.ctr_name_ = dv.name();

		allocator.commit();

		dv.setBranchingFactor(branching);

		try {
			out<<"insert data"<<endl;
			params.insert_ = true;

			params.data_ = 1;
			while (getSize(dv) < params.size_)
			{
				if (step)
				{
					params.step_ 		= getRandom(3);
				}

				params.block_size_ 	= 1 + getRandom(task_params->max_block_size_);

				Build(out, allocator, dv, &params);

				allocator.commit();

				params.data_++;

				params.pos_ 		= -1;
				params.page_step_ 	= -1;
			}

//			StoreAllocator(allocator, "vector.dump");

			out<<"remove data. Sumset contains "<<(getSize(dv)/1024)<<"K keys"<<endl;
			params.insert_ = false;

			for (Int c = 0; ; c++)
			{
				if (step)
				{
					params.step_ = getRandom(3);
				}

				BigInt size = getSize(dv);
				BigInt max_size = task_params->max_block_size_ <= size ? task_params->max_block_size_ : size;

				params.block_size_  = 1 + getBIRandom(max_size);
				params.page_step_ 	= getRandom(3);

				if (!remove(out, allocator, dv, &params))
				{
					break;
				}

				params.pos_ 		= -1;
				params.page_step_ 	= -1;

				allocator.commit();
			}

			out<<"Sumset.size = "<<(getSize(dv) / 1024)<<"K keys"<<endl;

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

		ArrayData data = CreateBuffer(array, params->block_size_, value);

		BigInt size = getSize(array);

		if (size == 0)
		{
			//insert buffer into an empty array
			auto iter = seek(array, 0);
			checkIterator(out, iter, MEMORIA_SOURCE);

			insert(iter, data);
			checkIterator(out, iter, MEMORIA_SOURCE);

			check(allocator, "insertion into an empty array failed. See the dump for details.", MEMORIA_SOURCE);

			auto iter1 = seek(array, 0);
			checkBufferWritten(iter1, data, "Failed to read and compare buffer from array", MEMORIA_SOURCE);
		}
		else {
			if (step == 0)
			{
				//insert at the start of the array
				auto iter = seek(array, 0);
				checkIterator(out, iter, MEMORIA_SOURCE);

				BigInt len = getSize(array);
				if (len > 100) len = 100;

				ArrayData postfix = CreateBuffer(array, len, 0);

				Read(iter, postfix);

				checkIterator(out, iter, MEMORIA_SOURCE);

				Skip(iter, -len);
				checkIterator(out, iter, MEMORIA_SOURCE);

				insert(iter, data);
				checkIterator(out, iter, MEMORIA_SOURCE);

				check(allocator, "insertion at the start of the array failed. See the dump for details.", 	MEMORIA_SOURCE);

				Skip(iter, -data.size()/getElementSize(array));
				checkIterator(out, iter, MEMORIA_SOURCE);

				checkBufferWritten(iter, data, "Failed to read and compare buffer from array", 				MEMORIA_SOURCE);
				checkIterator(out, iter, MEMORIA_SOURCE);

				checkBufferWritten(iter, postfix, "Failed to read and compare buffer postfix from array", 	MEMORIA_SOURCE);
				checkIterator(out, iter, MEMORIA_SOURCE);
			}
			else if (step == 1)
			{
				//insert at the end of the array
				BigInt len = getSize(array);

				auto iter = seek(array, len);
				checkIterator(out, iter, MEMORIA_SOURCE);

				if (len > 100) len = 100;

				ArrayData prefix = CreateBuffer(array, len, 0);
				Skip(iter, -len);
				checkIterator(out, iter, MEMORIA_SOURCE);

				Read(iter, prefix);
				checkIterator(out, iter, MEMORIA_SOURCE);

				insert(iter, data);
				checkIterator(out, iter, MEMORIA_SOURCE);

				check(allocator, "insertion at the end of the array failed. See the dump for details.", MEMORIA_SOURCE);

				Skip(iter, -data.size()/getElementSize(array) - len);
				checkIterator(out, iter, MEMORIA_SOURCE);

				checkBufferWritten(iter, prefix, "Failed to read and compare buffer prefix from array", MEMORIA_SOURCE);
				checkIterator(out, iter, MEMORIA_SOURCE);

				checkBufferWritten(iter, data, "Failed to read and compare buffer from array", 			MEMORIA_SOURCE);
				checkIterator(out, iter, MEMORIA_SOURCE);
			}
			else {
				//insert in the middle of the array

				if (params->pos_ == -1) params->pos_ = getRandomPosition(array);

				Int pos = params->pos_;

				auto iter = seek(array, pos);
				checkIterator(out, iter, MEMORIA_SOURCE);

				if (params->page_step_ == -1) params->page_step_ = getRandom(2);

				if (params->page_step_ == 0)
				{
					Skip(iter, -getLocalPosition(iter));
					checkIterator(out, iter, MEMORIA_SOURCE);
					pos = getPosition(iter);
				}

				BigInt prefix_len = pos;
				if (prefix_len > 100) prefix_len = 100;

				BigInt postfix_len = getSize(array) - pos;
				if (postfix_len > 100) postfix_len = 100;

				ArrayData prefix	= CreateBuffer(array, prefix_len, 0);
				ArrayData postfix	= CreateBuffer(array, postfix_len, 0);

				Skip(iter, -prefix_len);
				checkIterator(out, iter, MEMORIA_SOURCE);

				Read(iter, prefix);
				checkIterator(out, iter, MEMORIA_SOURCE);

				Read(iter, postfix);
				checkIterator(out, iter, MEMORIA_SOURCE);

				Skip(iter, -postfix_len);
				checkIterator(out, iter, MEMORIA_SOURCE);

				insert(iter, data);
				checkIterator(out, iter, MEMORIA_SOURCE);

				check(allocator, "insertion at the middle of the array failed. See the dump for details.", 	MEMORIA_SOURCE);

				Skip(iter, - data.size()/getElementSize(array) - prefix_len);
				checkIterator(out, iter, MEMORIA_SOURCE);

				checkBufferWritten(iter, prefix, 	"Failed to read and compare buffer prefix from array", 	MEMORIA_SOURCE);
				checkIterator(out, iter, MEMORIA_SOURCE);

				checkBufferWritten(iter, data, 		"Failed to read and compare buffer from array", 		MEMORIA_SOURCE);
				checkIterator(out, iter, MEMORIA_SOURCE);

				checkBufferWritten(iter, postfix, 	"Failed to read and compare buffer postfix from array", MEMORIA_SOURCE);
				checkIterator(out, iter, MEMORIA_SOURCE);
			}
		}
	}

	bool remove(ostream& out, Allocator& allocator, Ctr& array, ReplayParamType* params)
	{
		Int step = params->step_;

		params->cnt_++;

		if (getSize(array) < 200)
		{
			auto iter = array.Begin();
			checkIterator(out, iter, MEMORIA_SOURCE);

			remove(iter, getSize(array));
			checkIterator(out, iter, MEMORIA_SOURCE);

			check(allocator, "remove ByteArray", MEMORIA_SOURCE);
			return getSize(array) > 0;
		}
		else {
			BigInt size = params->block_size_;

			if (step == 0)
			{
				//remove at the start of the array
				auto iter = seek(array, 0);
				checkIterator(out, iter, MEMORIA_SOURCE);

				BigInt len = getSize(array) - size;
				if (len > 100) len = 100;

				ArrayData postfix(len * getElementSize(array));
				Skip(iter, size);
				checkIterator(out, iter, MEMORIA_SOURCE);

				Read(iter, postfix);
				checkIterator(out, iter, MEMORIA_SOURCE);

				Skip(iter, -len - size);
				checkIterator(out, iter, MEMORIA_SOURCE);

				remove(iter, size);
				checkIterator(out, iter, MEMORIA_SOURCE);

				check(allocator, "Removing region at the start of the array failed. See the dump for details.", MEMORIA_SOURCE);

				checkBufferWritten(iter, postfix, "Failed to read and compare buffer postfix from array", 		MEMORIA_SOURCE);
				checkIterator(out, iter, MEMORIA_SOURCE);
			}
			else if (step == 1)
			{
				//remove at the end of the array
				auto iter = seek(array, getSize(array) - size);
				checkIterator(out, iter, MEMORIA_SOURCE);

				BigInt len = getPosition(iter);
				if (len > 100) len = 100;

				ArrayData prefix(len * getElementSize(array));
				Skip(iter, -len);
				checkIterator(out, iter, MEMORIA_SOURCE);

				Read(iter, prefix);
				checkIterator(out, iter, MEMORIA_SOURCE);

				remove(iter, size);
				checkIterator(out, iter, MEMORIA_SOURCE);

				check(allocator, "Removing region at the end of the array failed. See the dump for details.", 	MEMORIA_SOURCE);

				Skip(iter, -len);
				checkIterator(out, iter, MEMORIA_SOURCE);

				checkBufferWritten(iter, prefix, "Failed to read and compare buffer prefix from array", 		MEMORIA_SOURCE);
				checkIterator(out, iter, MEMORIA_SOURCE);
			}
			else {
				//remove at the middle of the array
				if (params->pos_ == -1) params->pos_ = getRandomPosition(array);

				Int pos = params->pos_;

				auto iter = seek(array, pos);

				if (params->page_step_ == -1) params->page_step_ = getRandom(2);

				if (params->page_step_ == 0)
				{
					Skip(iter, -getLocalPosition(iter));
					checkIterator(out, iter, MEMORIA_SOURCE);

					pos = getPosition(iter);
				}

				if (pos + size > getSize(array))
				{
					size = getSize(array) - pos - 1;
				}

				BigInt prefix_len = pos;
				if (prefix_len > 100) prefix_len = 100;

				BigInt postfix_len = getSize(array) - (pos + size);
				if (postfix_len > 100) postfix_len = 100;

				ArrayData prefix(prefix_len * getElementSize(array));
				ArrayData postfix(postfix_len * getElementSize(array));

				Skip(iter, -prefix_len);
				checkIterator(out, iter, MEMORIA_SOURCE);

				Read(iter, prefix);
				checkIterator(out, iter, MEMORIA_SOURCE);

				Skip(iter, size);
				checkIterator(out, iter, MEMORIA_SOURCE);

				Read(iter, postfix);
				checkIterator(out, iter, MEMORIA_SOURCE);

				Skip(iter, -postfix_len - size);
				checkIterator(out, iter, MEMORIA_SOURCE);

				remove(iter, size);
				checkIterator(out, iter, MEMORIA_SOURCE);

				check(allocator, "Removing region at the middle of the array failed. See the dump for details.", 	MEMORIA_SOURCE);

				Skip(iter, -prefix_len);
				checkIterator(out, iter, MEMORIA_SOURCE);

				checkBufferWritten(iter, prefix, 	"Failed to read and compare buffer prefix from array", 			MEMORIA_SOURCE);
				checkIterator(out, iter, MEMORIA_SOURCE);

				checkBufferWritten(iter, postfix, 	"Failed to read and compare buffer postfix from array", 		MEMORIA_SOURCE);
				checkIterator(out, iter, MEMORIA_SOURCE);
			}

			return getSize(array) > 0;
		}

		return false;
	}


	virtual void checkIterator(ostream& out, Iterator& iter, const char* source)
	{
		checkIteratorPrefix(out, iter, source);

		auto& path = iter.path();

		for (Int level = path.getSize() - 1; level > 0; level--)
		{
			bool found = false;

			for (Int idx = 0; idx < path[level]->children_count(); idx++)
			{
				ID id = iter.model().getINodeData(path[level].node(), idx);
				if (id == path[level - 1]->id())
				{
					if (path[level - 1].parent_idx() != idx)
					{
						iter.Dump(out);
						throw TestException(source, SBuf()<<"Invalid parent-child relationship for node:"<<path[level]->id()<<" child: "<<path[level - 1]->id()<<" idx="<<idx<<" parent_idx="<<path[level-1].parent_idx());
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
				throw TestException(source, SBuf()<<"Child: "<<path[level - 1]->id()<<" is not fount is it's parent, parent_idx="<<path[level - 1].parent_idx());
			}
		}


	}

	virtual void checkIteratorPrefix(ostream& out, Iterator& iter, const char* source)
	{
		Accumulator prefix;
		iter.ComputePrefix(prefix);

		if (iter.prefix(0) != prefix.key(0))
		{
			iter.Dump(out);
			throw TestException(source, SBuf()<<"Invalid prefix value. Iterator: "<<iter.prefix()<<" Actual: "<<prefix);
		}
	}

};

}


#endif
