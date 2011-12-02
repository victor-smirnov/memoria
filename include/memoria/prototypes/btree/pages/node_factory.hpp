
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTREE_PAGES_NODE_FACTORY_HPP
#define	_MEMORIA_PROTOTYPES_BTREE_PAGES_NODE_FACTORY_HPP


#include <memoria/core/pmap/packed_map.hpp>
#include <memoria/core/tools/reflection.hpp>
#include <memoria/prototypes/btree/pages/node_base.hpp>

namespace memoria    {
namespace btree     {


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
    static const int BLOCK_SIZE = Allocator::PAGE_SIZE - sizeof(Base) - 100;
    typedef PackedMapMetadata<BLOCK_SIZE> MapConstants;



public:

    struct MapMetadataTypes {
    	typedef typename Types::Key 	Key;
    	typedef typename Types::Value 	Value;
    	typedef typename Types::Key 	IndexKey;

    	static const long Indexes = Types::Indexes;
    	static const int BlockSize = BLOCK_SIZE;
    	static const int Children = 16;
    };

    typedef PackedMapTypes<MapMetadataTypes, Types::PackedMapType> 				MapTypes;

    typedef PackedMap<MapTypes>													Map;

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

    static bool is_abi_compatible() {
        return reflection_->IsAbiCompatible();
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

    void Reindex()
    {
        map().Reindex();
    }

    Int data_size() const
    {
        Int size = map().size();
        Me* me = NULL;
        return (Int)(BigInt)&me->map().key(size);
    }

    template <template <typename> class FieldFactory>
    void BuildFieldsList(MetadataList &list, Long &abi_ptr) const {
        Base::template BuildFieldsList<FieldFactory>(list, abi_ptr);
        FieldFactory<Map>::create(list, map(), "MAP", abi_ptr);
    }

    template <typename PageType>
    void CopyFrom(const PageType* page)
    {
        Base::CopyFrom(page);

        map().size()   = page->map().size();

        for (Int c = 0; c < page->map().size(); c++)
        {
            for (Int d = 0; d < INDEXES; d++)
            {
                map().key(d, c) = page->map().key(d, c);
            }
            map().data(c) = page->map().data(c);
        }

        for (Int c = map().size(); c < map().max_size(); c++)
        {
            for (Int d = 0; d < INDEXES; d++)
            {
                map().key(d, c) = 0;
            }
            map().data(c) = 0;
        }
    }

    static Int get_page_size(const void* buf) {
        const Me* me = static_cast<const Me*>(buf);
        return me->data_size();
    }

    static Int Init() {
        if (reflection_ == NULL)
        {
            Map::Init();

            Long abi_ptr = 0;
            Me* me = 0;
            MetadataList list;
            me->BuildFieldsList<FieldFactory>(list, abi_ptr);

            Int hash0 = 1234567 + Descriptor::Root + 2 * Descriptor::Leaf + 4 * Descriptor::Level + 8 * Types::Indexes + 16 * Types::PackedMapType;

            Int attrs = BTREE + Descriptor::Root * ROOT + Descriptor::Leaf * LEAF;

            reflection_ = new PageMetadataImpl("BTREE_PAGE", list, attrs, hash0, &get_page_size, Allocator::PAGE_SIZE);
        }
        else {}

        return reflection_->Hash();
    }

    void *operator new(size_t size, Allocator &allocator) {
    	typename Allocator::Page *adr = allocator.create_new();
    	cout<<"NodeFactory: "<<adr<<" "<<adr->id().value()<<endl;
        return adr;
    }

    void operator delete(void *buf, size_t size) {}
};


template <
        typename Types
>
PageMetadata* NodePage<Types>::reflection_ = NULL;


#pragma pack()

}
}

#endif
