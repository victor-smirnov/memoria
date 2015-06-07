// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CORE_PACKED_MAP_FSE_TREE_BASE_HPP_
#define MEMORIA_CORE_PACKED_MAP_FSE_TREE_BASE_HPP_


#include <memoria/core/packed/tree/packed_fse_tree.hpp>
#include <memoria/core/packed/tree/packed_tree_tools.hpp>
#include <memoria/core/packed/tools/packed_tools.hpp>
#include <memoria/metadata/page.hpp>

#include <utility>

namespace memoria {





template <Int Blocks_, typename Key> class PackedMapTreeBase;

template <Int Blocks_, typename Key_>
class PackedMapTreeBase: public PackedAllocator {

    typedef PackedAllocator                                                     Base;
public:

    typedef PackedMapTreeBase<Blocks_, Key_>                                    MyType;

    typedef PkdFTree<
            Packed2TreeTypes<
                Key_, typename PackedTreeIndexTF<Key_>::Type, Blocks_
            >
    >                                                                           Tree;

    typedef Key_                                                                Key;

    typedef typename Tree::Values                                               Values;
    typedef typename Tree::Values2                                              Values2;

    typedef typename Tree::IndexValue                                           IndexValue;
    typedef typename Tree::ValueDescr                                           ValueDescr;

    static const Int TREE                                                       = 0;
    static const bool HasValue                                                  = true;
    static const Int Blocks                                                     = Blocks_;

    static const PackedSizeType TreeSizeType									= PkdStructSizeType<Tree>::Value;

    Tree* tree() {
        return Base::template get<Tree>(TREE);
    }

    const Tree* tree() const {
        return Base::template get<Tree>(TREE);
    }

    Int size() const
    {
        return tree()->size();
    }

    Key& key(Int key_num, Int idx)
    {
        return tree()->value(key_num, idx);
    }

    const Key& key(Int key_num, Int idx) const
    {
        return tree()->value(key_num, idx);
    }

    static Int tree_empty_size()
    {
        Int tree_empty_size = Tree::empty_size();
        return tree_empty_size;
    }

    static Int tree_block_size(Int size)
    {
        Int tree_block_size = Tree::packed_block_size(size);
        return tree_block_size;
    }


    void tree_init()
    {
        Base::template allocateEmpty<Tree>(TREE);
    }


    template <typename Lengths, typename Entry>
    static void computeTreeEntryDataLength(const Entry& entry, Lengths& lengths)
    {
        std::get<0>(lengths) += Tree::computeDataLength(entry.indexes());
    }


    template <typename Entry, typename MapSums>
    void insertTree(Int idx, const Entry& entry, MapSums& sums)
    {
        tree()->insert(idx, entry.indexes());

        sums[0] += 1;
        sums.sumAt(1, entry.indexes());
    }

    void insertTreeSpace(Int room_start, Int room_length)
    {
        tree()->insertSpace(room_start, room_length);
    }


    void removeTreeSpace(Int room_start, Int room_end)
    {
        tree()->remove(room_start, room_end);
        tree()->reindex();
    }

    void splitTreeTo(MyType* other, Int split_idx)
    {
        tree()->splitTo(other->tree(), split_idx);
    }

    void mergeTreeWith(MyType* other)
    {
        tree()->mergeWith(other->tree());
    }

    void reindexTree() {
        tree()->reindex();
    }

    void checkTree() const
    {
        tree()->check();
    }

    void dumpTree(std::ostream& out = std::cout) const
    {
        tree()->dump(out);
    }

    // ============================ IO =============================================== //

    template <typename DataSource>
    void insertTree(DataSource* src, Int idx, Int size)
    {
        tree()->insert(idx, size, [src](){return src->get().indexes();});
    }

    template <typename DataSource>
    void updateTree(DataSource* src, Int start, Int end)
    {
        tree()->update(start, end, [src](){return src->get().indexes();});
    }

    template <typename DataTarget>
    void readTree(DataTarget* tgt, Int start, Int end) const
    {
        tree()->read(start, end, [tgt](const Values& values) {
            auto current        = tgt->peek();
            current.indexes()   = values;
            tgt->put(current);
        });
    }


    // ============================ Serialization ==================================== //

    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        Base::generateDataEvents(handler);

        tree()->generateDataEvents(handler);
    }


    void serialize(SerializationData& buf) const
    {
        Base::serialize(buf);

        tree()->serialize(buf);
    }

    void deserialize(DeserializationData& buf)
    {
        Base::deserialize(buf);

        tree()->deserialize(buf);
    }
};



}

#endif
