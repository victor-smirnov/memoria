
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTREE_PAGES_NODE_FACTORY_HPP
#define _MEMORIA_PROTOTYPES_BTREE_PAGES_NODE_FACTORY_HPP

#include <memoria/core/types/static_md5.hpp>

#include <memoria/core/packed/packed_sum_tree.hpp>
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
    static const UInt VERSION                                                   = 1;
    static const Int  BranchingFactor                                           = PackedTreeBranchingFactor;
public:

    typedef NodePage<Types>                                                     Me;
    typedef PageStart<Types>                                                    Base;

private:

public:

//    struct MapTypes {
//        typedef typename Types::Key     Key;
//        typedef typename Types::Value   Value;
//        typedef typename Types::Key     IndexKey;
//
//        static const Int Blocks             = Types::Indexes;
//        static const Int BranchingFactor    = 32;
//
//        typedef Accumulators<Key, Blocks>   Accumulator;
//    };

    typedef PackedSumTree<
                PackedTreeTypes<
                    typename Types::Key,
                    typename Types::Key,
                    typename Types::Value,
                    Accumulators<typename Types::Key, Types::Indexes>,
                    Types::Indexes,
                    BranchingFactor
                >
    >                                                                           Map;

//    typedef PackedSumTree<MapTypes>                                           Map;

    typedef typename Types::Descriptor                                          Descriptor;



private:

    // Don't forget to update fields list
    // if type description is changed
    typedef typename MergeLists<
            typename Base::FieldsList,

            UIntValue<VERSION>,
            UIntValue<Descriptor::Root>,
            UIntValue<Descriptor::Leaf>,
            UIntValue<Descriptor::Level>,
            UIntValue<Types::Indexes>,
            typename Types::Name,
            UIntValue<BranchingFactor>,

            typename Map::FieldsList
    >::Result                                                                   FieldsList;

    static const UInt PAGE_HASH = md5::Md5Sum<typename TypeToValueList<FieldsList>::Type>::Result::Value32;

    Map map_;

    static PageMetadata *page_metadata_;

public:

    static const long INDEXES                                                   = Types::Indexes;

    typedef typename Types::Key                                                 Key;
    typedef typename Types::Value                                               Value;

    NodePage(): Base(), map_() {}

    static Int hash() {
        return PAGE_HASH;
    }

    static PageMetadata *page_metadata() {
        return page_metadata_;
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
        map_.size()      = map_size;
    }

    void inc_size(Int count)
    {
        Base::map_size() += count;
        map_.size()      += count;
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

        virtual void resize(const void* page, void* buffer, Int new_size) const
        {
            const Me* me = T2T<const Me*>(page);
            Me* tgt = T2T<Me*>(buffer);

            tgt->copyFrom(me);
            tgt->map().initByBlock(new_size - sizeof(Me));

            me->map().transferTo(&tgt->map());
            tgt->set_children_count(me->children_count());

            tgt->map().clearUnused();
            tgt->map().reindex();
        }

        virtual void generateDataEvents(
                        const void* page,
                        const DataEventsParams& params,
                        IPageDataEventHandler* handler
                     ) const
        {
            const Me* me = T2T<const Me*>(page);
            handler->startPage("BTREE_NODE");
            me->generateDataEvents(handler);
            handler->endPage();
        }

        virtual void generateLayoutEvents(
                        const void* page,
                        const LayoutEventsParams& params,
                        IPageLayoutEventHandler* handler
                     ) const
        {
            const Me* me = T2T<const Me*>(page);
            handler->startPage("BTREE_NODE");
            me->generateLayoutEvents(handler);
            handler->endPage();
        }
    };

    static Int initMetadata()
    {
        if (page_metadata_ == NULL)
        {
            Int attrs = BTREE + Descriptor::Root * ROOT + Descriptor::Leaf * LEAF;

            page_metadata_ = new PageMetadata("BTREE_PAGE", attrs, hash(), new PageOperations());
        }
        else {}

        return page_metadata_->hash();
    }
};


template <
        typename Types
>
PageMetadata* NodePage<Types>::page_metadata_ = NULL;


#pragma pack()

}
}

#endif
