
// Copyright Victor Smirnov, Ivan Yurchenko 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_ROOT_CONTAINER_CONTAINER_HPP
#define _MEMORIA_CONTAINERS_ROOT_CONTAINER_CONTAINER_HPP

#include <memoria/core/container/container.hpp>

#include <memoria/containers/kvmap/factory.hpp>


namespace memoria {

template <typename Types> struct RootCtrTypes;

template <typename Types>
class Ctr<RootCtrTypes<Types> >
{
	typedef typename Types::Profile			Profile;
	typedef typename Types::Allocator 		Allocator;
	typedef typename Allocator::Page		Page;
	typedef typename Page::ID				ID;

	typedef typename CtrTF<Profile, DefKVMap, DefKVMap>::Type	KeyValueMap;

	KeyValueMap		map_;

public:
	typedef typename KeyValueMap::Iterator					Iterator;


	Ctr(Allocator &allocator, BigInt name, bool create = false): map_(allocator, name, create, "memoria::Root")
	{ }


	Ctr(Allocator &allocator, const ID& root_id): map_(allocator, root_id, "memoria::Root")
	{ }

	//Public API goes here

	Iterator Begin()
	{
		return map_.Begin();
	}


	Iterator End()
	{
		return map_.End();
	}


	void set_root(const ID& id)
	{
		map_.set_root(id);
	}


	bool GetValue(BigInt name, Int key_idx, ID& id)
	{
		typename KeyValueMap::Value v;
		bool result = map_.GetValue(name, key_idx, v);

		if (result)
		{
			id = ID(v);
		}

		return result;
	}


	void SetValueForKey(BigInt name, const ID& id)
	{
		map_.SetValueForKey(name, id.value());
	}


	void RemoveByKey(BigInt name)
	{
		map_.RemoveByKey(name);
	}


	static Int Init()
	{
		Int salt = 123456;
		return KeyValueMap::Init(salt);
	}


	static Int hash()
	{
		return KeyValueMap::hash();
	}


	static ContainerMetadata * reflection()
	{
		return KeyValueMap::reflection();
	}


	bool Check(void* ptr)
	{
		return map_.Check(ptr);
	}

};


}
#endif
