
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINER_DBLMAP_I_CRUD_HPP
#define _MEMORIA_CONTAINER_DBLMAP_I_CRUD_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/idata.hpp>
#include <memoria/core/tools/dump.hpp>

#include <memoria/containers/dbl_map/dblmap_names.hpp>
#include <memoria/containers/dbl_map/dblmap_tools.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <iostream>

namespace memoria    {




MEMORIA_ITERATOR_PART_NO_CTOR_BEGIN(memoria::dblmap::ItrCRUDName)

    typedef Ctr<typename Types::CtrTypes>                       				Container;


    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBaseG                                            NodeBaseG;

    typedef typename Container::Value                                           Value;
    typedef typename Container::Key                                             Key;
    typedef typename Container::Accumulator                                     Accumulator;

    typedef typename Container::DataSource                                      DataSource;
    typedef typename Container::DataTarget                                      DataTarget;
    typedef typename Container::LeafDispatcher                                  LeafDispatcher;
    typedef typename Container::Position                                        Position;

    template <typename T>
    using Find2ndLEWalker = typename Container::Types::template FindLEWalker<T>;


    struct ReadValueFn {
        typedef Value ReturnType;
        typedef Value ResultType;

        template <typename Node>
        ReturnType treeNode(const Node* node, Int offset)
        {
            return node->template processStreamRtn<1>(*this, offset);
        }

        template <Int StreamIdx, typename StreamType>
        ResultType stream(const StreamType* obj, Int offset)
        {
            MEMORIA_ASSERT(offset, <, obj->size());

        	return obj->value(offset);
        }
    };

    Value value()
    {
        auto& self = this->self();
        MEMORIA_ASSERT_TRUE(self.stream() == 1);

        return LeafDispatcher::dispatchConstRtn(self.leaf(), ReadValueFn(), self.idx());
    }

    struct SetValueFn {
        template <typename Node>
        void treeNode(Node* node, Int offset, const Value& value)
        {
            node->template processStream<1>(*this, offset, value);
        }

        template <Int StreamIdx, typename StreamType>
        void stream(StreamType* obj, Int offset, const Value& value)
        {
        	MEMORIA_ASSERT(offset, <, obj->size());

        	obj->value(offset) = value;
        }
    };

    void setValue(const Value& value)
    {
        auto& self = this->self();
        MEMORIA_ASSERT_TRUE(self.stream() == 1);

        self.leaf().update();

        LeafDispatcher::dispatch(self.leaf(), SetValueFn(), self.idx(), value);
    }


    struct Key2Fn {
    	typedef Key ReturnType;
    	typedef Key ResultType;

    	template <typename Node>
    	ReturnType treeNode(const Node* node, Int offset)
    	{
    		return node->template processStreamRtn<1>(*this, offset);
    	}

    	template <Int StreamIdx, typename StreamType>
    	ResultType stream(const StreamType* obj, Int offset)
    	{
    		MEMORIA_ASSERT(offset, <, obj->size());
    		return obj->tree()->value(0, offset);
    	}
    };

    Key key2() const
    {
    	auto prefix = self().cache().second_prefix();
    	Key value 	= self().raw_key2();

    	return value + prefix;
    }

    Key raw_key2() const
    {
    	auto& self = this->self();
    	MEMORIA_ASSERT_TRUE(self.stream() == 1);
    	return LeafDispatcher::dispatchConstRtn(self.leaf(), Key2Fn(), self.idx());
    }


    bool find2ndLE(Key key)
    {
    	auto& self = this->self();

    	BigInt size = self.blob_size();

    	if (self.stream() == 0)
    	{
    		self.findData(0);
    	}
    	else
    	{
    		BigInt pos = self.pos();

    		if (pos > 0)
    		{
    			self.skipBw(pos);
    		}
    	}

    	if (size > 0)
    	{
    		BigInt offset = self.template _findFw<Find2ndLEWalker>(0, key);

    		self.cache().addToGlobalPos(offset);

    		if (offset < size)
    		{
    			return true;
    		}
    		else {
    			self.skipBw(offset - size);

    			return false;
    		}
    	}
    	else {
    		return false;
    	}
    }


    bool insert2nd(Key key, Value value)
    {
    	auto& self = this->self();

    	self.findData();

    	if (self.find2ndLE(key))
    	{
    		auto k = self.key2();

    		if (k != key)
    		{
    			this->insert2ndEntry(key, value);
    			return true;
    		}
    		else {
    			self.setValue(value);
    			return false;
    		}
    	}
    	else {
    		this->insert2ndEntry(key, value);
    		return true;
    	}
    }

private:

    struct AddKey2Fn {

    	template <typename Node>
    	void treeNode(Node* node, Int offset, Key key)
    	{
    		node->template processStream<1>(*this, offset, key);
    	}

    	template <Int StreamIdx, typename StreamType>
    	void stream(StreamType* obj, Int offset, Key key)
    	{
    		MEMORIA_ASSERT(offset, <, obj->size());

    		obj->tree()->value(0, offset) += key;
    		obj->reindex();
    	}
    };



    void insert2ndEntry(const Key& key, const Value& value)
    {
    	auto& self = this->self();

    	typedef StaticVector<Key, 1> 			KeyV;
    	typedef std::pair<KeyV, Value> 			IOValue;

    	KeyV key_v;

    	auto delta = key - self.cache().second_prefix();

    	key_v[0] = delta;

    	IOValue io_val(key_v, value);

    	ValueSource<IOValue> src(io_val);

    	self.insert(src);

    	if (self.pos() < self.blob_size())
    	{
    		LeafDispatcher::dispatch(self.leaf(), AddKey2Fn(), self.idx(), -delta);

    		Accumulator sums;

    		std::get<1>(sums)[1] = -delta;

    		self.ctr().updateParent(self.leaf(), sums);
    	}
    }



public:

    bool remove2nd(Key key)
    {
    	auto& self = this->self();

    	typedef StaticVector<Key, 1> 			KeyV;
    	typedef std::pair<KeyV, Value> 			IOValue;

    	self.findData();

    	BigInt pos = self.pos();
    	if (pos > 0)
    	{
    		self.skipBw(pos);
    	}

    	if (self.find2ndLE(key))
    	{
    		if (self.key2() == key)
    		{
    			auto delta = self.raw_key2();

    			self.remove(1);

    			if (self.pos() < self.blob_size())
    			{
    				if (self.idx() == self.leaf_size(1))
    				{
    					self.nextLeaf();
    				}

    				LeafDispatcher::dispatch(self.leaf(), AddKey2Fn(), self.idx(), delta);

    				Accumulator sums;

    				std::get<1>(sums)[1] = delta;

    				self.ctr().updateParent(self.leaf(), sums);
    			}

    			return true;
    		}
    		else {
    			return false;
    		}
    	}
    	else {
    		return false;
    	}
    }





    BigInt read(DataTarget& tgt)
    {
        auto& self = this->self();

        vmap::VectorMapTarget target(&tgt);

        Position pos;

        pos[1] = self.idx();

        return self.ctr().readStreams(self, pos, target)[1];
    }

    void insert(DataSource& src)
    {
        auto& self = this->self();

        MEMORIA_ASSERT_TRUE(self.stream() == 1);

        self.ctr().insertData(self, src);
    }


    void remove(BigInt size)
    {
        auto& self = this->self();

        MEMORIA_ASSERT_TRUE(self.stream() == 1);

        self.ctr().removeData(self, size);
    }




    struct UpdateFn {

        template <typename Node>
        void treeNode(Node* node, Int offset, const Accumulator& keys)
        {
            node->template processStream<0>(*this, offset, keys);
        }

        template <Int StreamIdx, typename StreamType>
        void stream(StreamType* obj, Int offset, const Accumulator& keys)
        {
            obj->updateUp(0, offset, std::get<0>(keys)[0]);
            obj->updateUp(1, offset, std::get<0>(keys)[1]);
            obj->reindex();
        }
    };

    void update(const Accumulator& accum)
    {
        auto& self = this->self();

        MEMORIA_ASSERT_TRUE(self.stream() == 0);

        NodeBaseG& leaf = self.leaf();

        leaf.update();
        LeafDispatcher::dispatch(leaf, UpdateFn(), self.idx(), accum);

        self.ctr().updateParent(leaf, accum);

        self.cache().addToEntry(
            std::get<0>(accum)[0],
            std::get<0>(accum)[1]
        );
    }

    void dump() const {
        auto cache = self().cache();

        cout<<"Cache: id=" <<cache.id()
                        <<" id_prefix="<<cache.id_prefix()
                        <<" id_entry="<<cache.id_entry()
                        <<" base="<<cache.blob_base()
                        <<" size="<<cache.size()
                        <<" idx="<<cache.entry_idx()
                        <<" entries="<<cache.entries()
                        <<" second_prefix="<<cache.second_prefix()
                        <<endl;

        cout<<" blob_size="<<self().blob_size()<<" Leaf ID="<<self().leaf()->id()
            <<" idx="<<self().idx()
            <<endl;

        Base::dump();
    }

    void dumpCache() const {
    	auto cache = self().cache();

    	cout<<"Cache: id=" <<cache.id()
                            		<<" id_prefix="<<cache.id_prefix()
                            		<<" id_entry="<<cache.id_entry()
                            		<<" base="<<cache.blob_base()
                            		<<" size="<<cache.size()
                            		<<" idx="<<cache.entry_idx()
                            		<<" entries="<<cache.entries()
                            		<<" second_prefix="<<cache.second_prefix()
                            		<<endl;

    	cout<<" blob_size="<<self().blob_size()<<" Leaf ID="<<self().leaf()->id()
    		<<" idx="<<self().idx()
    		<<endl;
    }


MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::dblmap::ItrCRUDName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS




}

#undef M_TYPE
#undef M_PARAMS

#endif
