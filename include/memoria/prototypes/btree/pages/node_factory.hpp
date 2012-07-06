
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTREE_PAGES_NODE_FACTORY_HPP
#define	_MEMORIA_PROTOTYPES_BTREE_PAGES_NODE_FACTORY_HPP


#include <memoria/core/pmap/packed_sum_tree.hpp>
#include <memoria/core/tools/reflection.hpp>
#include <memoria/prototypes/btree/pages/node_base.hpp>

#include <memoria/prototypes/bstree/tools.hpp>

namespace memoria    {
namespace btree      {


#pragma pack(1)

template <
        typename Types
>
class NodePage: public PageStart<Types>
{

public:

    typedef NodePage<Types>                                                     Me;
    typedef PageStart<Types>                       								Base;
    typedef typename Types::NodePageBase										BaseType0;

    typedef typename BaseType0::Allocator                                       Allocator;

private:

    //FIXME: Need more accurate allocation
//    static const int BLOCK_SIZE = Allocator::PAGE_SIZE - sizeof(Base) - 100;
//    typedef PackedMapMetadata<BLOCK_SIZE> MapConstants;



public:

//    struct MapMetadataTypes {
//    	typedef typename Types::Key 	Key;
//    	typedef typename Types::Value 	Value;
//    	typedef typename Types::Key 	IndexKey;
//
//    	static const long Indexes = Types::Indexes;
//    	static const int BlockSize = BLOCK_SIZE;
//    	static const int Children = 16;
//    };
//
//    typedef PackedMapTypes<MapMetadataTypes, Types::PackedMapType> 				MapTypes;


    struct MapTypes {
    	typedef typename Types::Key 	Key;
    	typedef typename Types::Value 	Value;
    	typedef typename Types::Key 	IndexKey;

    	static const Int Blocks 			= Types::Indexes;
    	static const Int BranchingFactor	= 32;

    	typedef Accumulators<Key, Blocks> 	Accumulator;
    };

    typedef PackedSumTree<MapTypes>												Map;

    typedef typename Types::Descriptor                                      	Descriptor;

private:

    Map map_;

    static PageMetadata *reflection_;

public:

    static const long INDEXES                                                   = Types::Indexes;

    typedef typename Types::Key                                             	Key;
    typedef typename Types::Value                                           	Value;

    NodePage(): Base(), map_() {}

    static Int hash() {
        return reflection_->Hash();
    }

    static PageMetadata *reflection() {
        return reflection_;
    }

    const Map &map() const
    {
        return map_;
    }

    Map &map()
    {
        return map_;
    }

    void reindex()
    {
        map().reindex();
    }

    Int data_size() const
    {
    	return sizeof(Me) + map_.getDataSize();
    }

    void set_children_count(Int map_size)
    {
    	Base::map_size() = map_size;
    	map_.size() 	 = map_size;
    }

    void inc_size(Int count)
    {
    	Base::map_size() += count;
    	map_.size() 	 += count;
    }

    void generateDataEvents(IPageDataEventHandler* handler) const
    {
    	Base::generateDataEvents(handler);
    	map_.generateDataEvents(handler);
    }

    template <template <typename> class FieldFactory>
    void serialize(SerializationData& buf) const
    {
    	Base::template serialize<FieldFactory>(buf);

    	FieldFactory<Map>::serialize(buf, map_);
    }

    template <template <typename> class FieldFactory>
    void deserialize(DeserializationData& buf)
    {
    	Base::template deserialize<FieldFactory>(buf);

    	FieldFactory<Map>::deserialize(buf, map_);
    }

    template <typename PageType>
    void CopyFrom(const PageType* page)
    {
        Base::CopyFrom(page);

        //FIXME: use page->size()
        //FIXME: use PackedTree facilities to copy data or memcpy()
        set_children_count(page->children_count());

        for (Int c = 0; c < page->children_count(); c++)
        {
            for (Int d = 0; d < INDEXES; d++)
            {
                map_.key(d, c) = page->map().key(d, c);
            }

            map_.data(c) = page->map().data(c);
        }

        for (Int c = this->children_count(); c < map_.maxSize(); c++)
        {
            for (Int d = 0; d < INDEXES; d++)
            {
                map_.key(d, c) = 0;
            }

            map().data(c) = 0;
        }
    }



    class PageOperations: public IPageOperations
    {
    	virtual Int serialize(const void* page, void* buf) const
    	{
    		const Me* me = T2T<const Me*>(page);

    		SerializationData data;
    		data.buf = T2T<char*>(buf);

    		me->template serialize<FieldFactory>(data);

    		return data.total;
    	}

    	virtual void deserialize(const void* buf, Int buf_size, void* page) const
    	{
    		Me* me = T2T<Me*>(page);

    		DeserializationData data;
    		data.buf = T2T<const char*>(buf);

    		me->template deserialize<FieldFactory>(data);
    	}

    	virtual Int getPageSize(const void *page) const
    	{
    		const Me* me = T2T<const Me*>(page);
    		return me->data_size();
    	}

    	virtual void generateDataEvents(const void* page, const DataEventsParams& params, IPageDataEventHandler* handler) const
    	{
    		const Me* me = T2T<const Me*>(page);
    		handler->StartPage("BTREE_NODE");
    		me->generateDataEvents(handler);
    		handler->EndPage();
    	}

    	virtual void GenerateLayoutEvents(const void* page, const LayoutEventsParams& params, IPageLayoutEventHandler* handler) const
    	{
    		const Me* me = T2T<const Me*>(page);
    		handler->StartPage("BTREE_NODE");
    		me->GenerateLayoutEvents(handler);
    		handler->EndPage();
    	}
    };

    static Int initMetadata()
    {
        if (reflection_ == NULL)
        {
            MetadataList list;

            Int hash0 = 1234567 + Descriptor::Root + 2 * Descriptor::Leaf + 4 * Descriptor::Level + 8 * Types::Indexes + 16 * Types::Name::Code;

            Int attrs = BTREE + Descriptor::Root * ROOT + Descriptor::Leaf * LEAF;

            reflection_ = new PageMetadata("BTREE_PAGE", list, attrs, hash0, new PageOperations(), Allocator::PAGE_SIZE);
        }
        else {}

        return reflection_->Hash();
    }
};


template <
        typename Types
>
PageMetadata* NodePage<Types>::reflection_ = NULL;


#pragma pack()

}
}

#endif
