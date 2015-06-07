
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINER_VECTORMAP_C_INSERT_HPP
#define _MEMORIA_CONTAINER_VECTORMAP_C_INSERT_HPP


#include <memoria/containers/vector_map/vmap_names.hpp>
#include <memoria/containers/vector_map/vmap_tools.hpp>

#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/core/tools/idata.hpp>

#include <vector>

namespace memoria    {

using namespace memoria::bt;

MEMORIA_CONTAINER_PART_BEGIN(memoria::vmap::CtrInsertName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Base::ID                                                   ID;

    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::NodeDispatcher                                       NodeDispatcher;
    typedef typename Base::RootDispatcher                                       RootDispatcher;
    typedef typename Base::LeafDispatcher                                       LeafDispatcher;
    typedef typename Base::BranchDispatcher                                    BranchDispatcher;
    typedef typename Base::DefaultDispatcher                                    DefaultDispatcher;


    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::Accumulator                                         Accumulator;
    typedef typename Types::Position                                            Position;

    typedef typename Types::CtrSizeT                                            CtrSizeT;

    typedef typename Types::DataSource                                          DataSource;
    typedef typename Types::DataTarget                                          DataTarget;

    static const Int Streams                                                    = Types::Streams;


    void insertData(Iterator& iter, DataSource& data);
    void insert(Iterator& iter, BigInt id, DataSource& data);

    void splitLeaf(Iterator& iter, Int split_idx = -1);
    void splitLeafData(Iterator& iter, Int split_idx = -1);

private:




    template <typename EntryData>
    void insertMapEntry(Iterator& iter, const EntryData&);

    void insertDataInternal(Iterator& iter, const Position& idx, DataSource& data);
    void insertDataInternal1(Iterator& iter, const Position& idx, DataSource& data);

MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::vmap::CtrInsertName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS




M_PARAMS
void M_TYPE::insertData(Iterator& iter, DataSource& data)
{
    auto& self = this->self();

    MEMORIA_ASSERT_TRUE(iter.stream() == 1);

    CtrSizeT data_size = data.getRemainder();
    CtrSizeT pos       = iter.pos();

    NodeBaseG& leaf = iter.leaf();

    Int idx         = iter.idx();



    if (self.checkCapacities(leaf, {0, data_size}) || self.isNodeEmpty(leaf))
    {
        insertDataInternal1(iter, {-1, idx}, data);
    }
    else
    {
        Int entry_idx   = iter.cache().entry_idx();
        Int leaf_size   = iter.leaf_size(0);

        if (iter.blob_size() > 0)
        {
            if (leaf_size == 0)
            {
                if (idx < iter.leaf_size(1))
                {
                    splitLeafData(iter, idx);
                    insertDataInternal1(iter, {0, 0}, data);
                }
                else {
                    insertDataInternal1(iter, {0, idx}, data);
                }
            }
            else {
                Int first_entry_data_offset = iter.data_offset_for(0);

                NodeBaseG right;

                if (idx < first_entry_data_offset)
                {
                    right = self.splitLeafP(iter.leaf(), {0, idx});

                    insertDataInternal1(iter, {0, idx}, data);
                }
                else if (idx == first_entry_data_offset && pos > 0)
                {
                    right = self.splitLeafP(iter.leaf(), {0, idx});

                    insertDataInternal1(iter, {0, idx}, data);
                }
                else
                {
                    right = self.splitLeafP(iter.leaf(), {entry_idx + 1, idx});
                    iter.cache().setEntries(iter.leaf_size(0));

                    insertDataInternal1(iter, {0, idx}, data);
                }
            }
        }
        else {
            // -1 added for DblMap
            if (entry_idx < leaf_size - 1)
            {
                auto right = self.splitLeafP(iter.leaf(), {entry_idx + 1, idx});
                iter.cache().setEntries(iter.leaf_size(0));

                insertDataInternal1(iter, {0, idx}, data);
            }
            else {
                insertDataInternal1(iter, {0, idx}, data);
            }
        }
    }

    iter.findEntry();

    Accumulator accum;

    std::get<0>(accum)[1] = data_size;

    iter.update(accum);

    iter.seek(pos + data_size);

    self.addTotalKeyCount({0, data_size});
}



M_PARAMS
void M_TYPE::insert(Iterator& iter, BigInt id, DataSource& src)
{
    auto& self = this->self();

    BigInt id_entry_value = id - iter.cache().id_prefix();

    std::pair<BigInt, CtrSizeT> pair(id_entry_value, src.getSize());

    NodeBaseG& leaf = iter.leaf();

    bool at_the_end = iter.isEnd();

    CtrSizeT data_size = src.getRemainder();

    if (self.checkCapacities(leaf, {1, data_size}) || self.isNodeEmpty(leaf))
    {
        insertMapEntry(iter, pair);
        iter.seekLocal();
        insertDataInternal(iter, {-1, iter.idx()}, src);
        iter.skipFw(data_size);
    }
    else
    {
        Int map_stream_size = iter.leafSize(0);

        if (iter.idx() < map_stream_size)
        {
            self.splitLeaf(iter, iter.idx());

            insertMapEntry(iter, pair);

            iter.seekLocal();

            insertDataInternal(iter, {-1, iter.idx()}, src);
            iter.skipFw(data_size);
        }
        else {
            iter--;

            CtrSizeT blob_size    = iter.blob_size();
            Int leaf_data_size  = iter.leafSize(1);
            Int blob_offset     = iter.data_offset();

            if (blob_offset + blob_size > leaf_data_size)
            {
                iter.seek(blob_size);

                Int capacity = iter.leaf_capacity(0);

                if (capacity == 1)
                {
                    if (src.getSize() > 0)
                    {
                        Int data_capacity = iter.leaf_capacity({1, 0}, 1);
                        if (data_capacity == 0)
                        {
                            self.splitLeafData(iter);
                        }
                    }
                }
                else if (capacity == 0)
                {
                    self.splitLeafData(iter);
                }

                Int data_pos = iter.idx();

                iter.idx()      = 0;
                iter.stream()   = 0;

                Base::insertEntry(iter, pair);

                iter.cache().add(pair.first, pair.second, iter.idx());

                iter.cache().setEntries(iter.leaf_size(0));

                iter.idx()      = data_pos;
                iter.stream()   = 1;

                self.insertDataInternal(iter, {-1, iter.idx()}, src);
                iter.skipFw(data_size);
            }
            else {
                MEMORIA_ASSERT_TRUE(blob_offset + blob_size == leaf_data_size);

                iter++;

                Int data_capacity = iter.leaf_capacity({1, 0}, 1);

                if (data_capacity > 0)
                {
                    insertMapEntry(iter, pair);

                    iter.seekLocal();

                    insertDataInternal(iter, {-1, iter.idx()}, src);
                    iter.skipFw(data_size);
                }
                else {
                    Int map_capacity = iter.leaf_capacity(0);

                    if (map_capacity > 0 && data_size == 0)
                    {
                        insertMapEntry(iter, pair);
                    }
                    else {
                        self.splitLeaf(iter, iter.idx());

                        insertMapEntry(iter, pair);

                        iter.stream() = 1;
                        iter.idx()    = 0;

                        insertDataInternal(iter, {-1, iter.idx()}, src);
                        iter.skipFw(data_size);
                    }
                }
            }
        }
    }

    self.addTotalKeyCount({0, data_size});

    if (!at_the_end)
    {
        iter.findEntry();

        iter++;

        Accumulator accum;

        std::get<0>(accum)[0] = -id_entry_value;

        iter.update(accum);

        iter--;
    }
}



M_PARAMS
void M_TYPE::splitLeaf(Iterator& iter, Int split_idx)
{
    auto& self = this->self();

    if (split_idx == -1)
    {
        split_idx = iter.leaf_size(0) / 2;
    }

//  UBigInt active_streams = self.getActiveStreams(iter.leaf());

    if (split_idx < iter.idx())
    {
        Int data_offset = iter.data_offset_for(split_idx);

        auto right = self.splitLeafP(iter.leaf(), {split_idx, data_offset});

        iter.idx() -= split_idx;
    }
    else if (split_idx == iter.idx())
    {
        Int data_offset = iter.data_offset();

//      auto right = iter.path();

        bool return_right_path = split_idx == iter.leaf_size(0);

        auto right = self.splitLeafP(iter.leaf(), {split_idx, data_offset});

        if (return_right_path)
        {
            iter.leaf() = right;
            iter.idx()  = 0;

            iter.cache().setEntryIdx(iter.idx());
        }
        else {
            iter.cache().set(0, 0, iter.idx(), 0, iter.cache().blob_base());
        }
    }
    else {
        Int data_offset = iter.data_offset_for(split_idx);

        auto right = self.splitLeafP(iter.leaf(), {split_idx, data_offset});
    }

    iter.cache().setEntries(iter.leaf_size(0));
}



M_PARAMS
template <typename EntryData>
void M_TYPE::insertMapEntry(Iterator &iter, const EntryData& entry)
{
    Base::insertEntry(iter, entry);
    iter.cache().set(entry.first, entry.second, iter.idx(), iter.leaf_size(0), iter.cache().blob_base());
}


M_PARAMS
void M_TYPE::splitLeafData(Iterator& iter, Int split_idx)
{
    auto& self = this->self();

    if (split_idx == -1)
    {
        split_idx = iter.idx() / 2;
    }

    auto right = self.splitLeafP(iter.leaf(), {0, split_idx});

    iter.leaf() = right;

    if (split_idx <= iter.idx())
    {
        iter.idx() -= split_idx;
    }
}



M_PARAMS
void M_TYPE::insertDataInternal(Iterator& iter, const Position& idx, DataSource& data)
{
    if (data.getSize() > 0)
    {
        auto& self = this->self();
        auto& ctr  = self;

        vmap::VectorMapSource source(&data);

        typename Base::DefaultSubtreeProvider provider(self, {0, data.getRemainder()}, source);

        Position idx0 = idx;

        ctr.insertSubtree(iter.leaf(), idx0, provider);

        if (iter.isEof())
        {
            iter.nextLeaf();
        }
    }
}


M_PARAMS
void M_TYPE::insertDataInternal1(Iterator& iter, const Position& idx, DataSource& data)
{
    if (data.getSize() > 0)
    {
        auto& self = this->self();
        auto& ctr  = self;

        vmap::VectorMapSource source(&data);

        typename Base::DefaultSubtreeProvider provider(self, {0, data.getRemainder()}, source);

        Position idx0 = idx;

        ctr.insertSubtree(iter.leaf(), idx0, provider);

        MEMORIA_ASSERT(data.getRemainder(), ==, 0);
    }
}


#undef M_PARAMS
#undef M_TYPE

}


#endif
