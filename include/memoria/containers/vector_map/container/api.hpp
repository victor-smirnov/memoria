
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
		if (!is_iter.isEnd())
		{
			auto ba_iter = me()->array().End();

			ba_iter.skip(-is_iter.getRawKey(1));

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

		is_iter.init();

		BigInt 	data_pos 	= is_iter.prefix(1);
		bool	end			= is_iter.isEnd();
		bool 	exists 		= end ? false : (is_iter.getKey(0) == key);

		auto ba_iter = me()->array().seek(data_pos);

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
			return me()->create(key);
		}
	}

	Iterator create()
	{
		auto is_iter = me()->set().End();

		IdxsetAccumulator keys;

		keys[0] = 1;

		me()->set().insertEntry(is_iter, keys);

		auto ba_iter = me()->array().End();
		return Iterator(*me(), is_iter, ba_iter);
	}

	void createNew(Iterator& iter)
	{
		IdxsetAccumulator keys;

		keys[0] = 1;

		me()->set().insertEntry(iter.is_iter(), keys);

		if (!iter.ba_iter().isEof())
		{
			iter.ba_iter().skip(me()->array().size() - iter.ba_iter().pos());
		}
	}

	Iterator create(BigInt key)
	{
		auto is_iter = me()->set().findLT(key, 0);

		//FIXME: set_.findLT() have to return properly initialized iterator
		//is_iter.init();

		BigInt 	data_pos 	= is_iter.prefix(1);
		bool	end			= is_iter.isEnd();
		bool 	exists 		= end ? false : (is_iter.getKey(0) == key);

		auto ba_iter = me()->array().seek(data_pos);

		if (exists)
		{
			return Iterator(*me(), is_iter, ba_iter, true);
		}
		else {
			BigInt delta = key - is_iter.prefix(0);


			IdxsetAccumulator keys;
			keys.key(0) = delta;


			if (is_iter.isNotEnd())
			{
				me()->set().updateUp(is_iter.path(), 0, is_iter.key_idx(), -keys);
			}

			me()->set().insertEntry(is_iter, keys);

//			auto ba_iter = me()->array().seek(data_pos);
			return Iterator(*me(), is_iter, ba_iter, false);
		}
	}

	bool remove(BigInt key)
	{
		auto is_iter = me()->set().findLE(key, 0);

		//FIXME: set_.findLT() have to return properly initialized iterator
//		is_iter.init();

		bool	end			= is_iter.isEnd();
		bool 	exists 		= end ? false : (is_iter.getKey(0) == key);

		if (exists)
		{
			BigInt 	data_pos 	= is_iter.prefix(1);
			BigInt 	size		= is_iter.getRawKey(1);

			IdxsetAccumulator accum;

			me()->set().removeEntry(is_iter, accum);

			if (!is_iter.isEnd())
			{
				accum[1] = 0;
				is_iter.updateUp(accum);
			}

			auto 	ba_iter 	= me()->array().seek(data_pos);
			ba_iter.remove(size);

			return true;
		}
		else {
			return false;
		}
	}

	void removeByIndex(BigInt blob_index)
	{
		auto is_iter 	= me()->set().getByIndex(blob_index);

		BigInt pos 		= is_iter.prefix(1);
		BigInt size 	= is_iter.getRawKey(1);

		auto ba_iter 	= me()->array().seek(pos);

		ba_iter.remove(size);
		me()->set().removeEntry(is_iter);
	}

	BigInt count()
	{
		return me()->set().getSize();
	}

	BigInt size()
	{
		return me()->array().size();
	}


MEMORIA_CONTAINER_PART_END

}


#endif
