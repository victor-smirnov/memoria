
// Copyright Victor Smirnov 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_MODELS_VECTOR_MAP_CONTAINER_API_HPP
#define	_MEMORIA_MODELS_VECTOR_MAP_CONTAINER_API_HPP

#include <memoria/core/container/container.hpp>
#include <memoria/containers/vector_map/names.hpp>

namespace memoria    {

using namespace memoria::btree;

MEMORIA_CONTAINER_PART_BEGIN(memoria::vector_map::CtrApiName)

    typedef typename Base::Iterator                                             Iterator;

	typedef typename Base::IdxsetAccumulator                        			IdxsetAccumulator;

	typedef typename Base::Key													Key;
	typedef typename Base::ISValue												ISValue;

	static const Int IS_Indexes													= Base::BA_Indexes;
	static const Int BA_Indexes													= Base::IS_Indexes;

	Iterator Begin()
	{
		return Iterator(*me(), me()->set().Begin(), me()->array().Begin());
	}

	Iterator begin()
	{
		return Iterator(*me(), me()->set().Begin(), me()->array().Begin());
	}

	Iterator End()
	{
		auto is_iter = me()->set().End();
		if (!is_iter.IsEnd())
		{
			auto ba_iter = me()->array().End();

			ba_iter.Skip(-is_iter.getRawKey(1));

			return Iterator(*me(), is_iter, ba_iter);
		}
		else {
			return Iterator(*me(), is_iter, me()->array().End());
		}
	}

	Iterator end()
	{
		Iterator iter(*me());
		iter.type() = Iterator::END;
		return iter;
	}

	IterEndMark endm()
	{
		return IterEndMark();
	}

	Iterator find(BigInt key)
	{
		auto is_iter = me()->set().findLE(key, 0);  // FIXME check for bounds (for_insert)

		is_iter.Init();

		BigInt 	data_pos 	= is_iter.prefix(1);
		bool	end			= is_iter.IsEnd();
		bool 	exists 		= end ? false : (is_iter.getKey(0) == key);

		auto ba_iter = me()->array().Seek(data_pos);

		return Iterator(*me(), is_iter, ba_iter, exists);
	}

	Iterator operator[](BigInt key)
	{
		Iterator iter = find(key);

		if (iter.exists())
		{
			return iter;
		}
		else {
			return me()->Create(key);
		}
	}

	Iterator Create()
	{
		auto is_iter = me()->set().End();

		IdxsetAccumulator keys;

		keys[0] = 1;

		me()->set().InsertEntry(is_iter, keys);

		auto ba_iter = me()->array().End();
		return Iterator(*me(), is_iter, ba_iter);
	}

	void CreateNew(Iterator& iter)
	{
		IdxsetAccumulator keys;

		keys[0] = 1;

		me()->set().InsertEntry(iter.is_iter(), keys);

		if (!iter.ba_iter().IsEof())
		{
			iter.ba_iter().Skip(me()->array().Size() - iter.ba_iter().pos());
		}
	}

	Iterator Create(BigInt key)
	{
		auto is_iter = me()->set().findLT(key, 0);

		//FIXME: set_.findLT() have to return properly initialized iterator
		//is_iter.Init();

		BigInt 	data_pos 	= is_iter.prefix(1);
		bool	end			= is_iter.IsEnd();
		bool 	exists 		= end ? false : (is_iter.getKey(0) == key);

		auto ba_iter = me()->array().Seek(data_pos);

		if (exists)
		{
			return Iterator(*me(), is_iter, ba_iter, true);
		}
		else {
			BigInt delta = key - is_iter.prefix(0);


			IdxsetAccumulator keys;
			keys.key(0) = delta;


			if (is_iter.IsNotEnd())
			{
				me()->set().UpdateUp(is_iter.path(), 0, is_iter.key_idx(), -keys);
			}

			me()->set().InsertEntry(is_iter, keys);

//			auto ba_iter = me()->array().Seek(data_pos);
			return Iterator(*me(), is_iter, ba_iter, false);
		}
	}

	bool Remove(BigInt key)
	{
		auto is_iter = me()->set().findLE(key, 0);

		//FIXME: set_.findLT() have to return properly initialized iterator
//		is_iter.Init();

		bool	end			= is_iter.IsEnd();
		bool 	exists 		= end ? false : (is_iter.getKey(0) == key);

		if (exists)
		{
			BigInt 	data_pos 	= is_iter.prefix(1);
			BigInt 	size		= is_iter.getRawKey(1);

			IdxsetAccumulator accum;

			me()->set().RemoveEntry(is_iter, accum);

			if (!is_iter.IsEnd())
			{
				accum[1] = 0;
				is_iter.UpdateUp(accum);
			}

			auto 	ba_iter 	= me()->array().Seek(data_pos);
			ba_iter.Remove(size);

			return true;
		}
		else {
			return false;
		}
	}

	void RemoveByIndex(BigInt blob_index)
	{
		auto is_iter 	= me()->set().getByIndex(blob_index);

		BigInt pos 		= is_iter.prefix(1);
		BigInt size 	= is_iter.getRawKey(1);

		auto ba_iter 	= me()->array().Seek(pos);

		ba_iter.Remove(size);
		me()->set().RemoveEntry(is_iter);
	}

	BigInt Count()
	{
		return me()->set().getSize();
	}

	BigInt Size()
	{
		return me()->array().Size();
	}


MEMORIA_CONTAINER_PART_END

}


#endif
