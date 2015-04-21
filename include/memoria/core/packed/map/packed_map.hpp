// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CORE_PACKED_PACKED_MAP_HPP_
#define MEMORIA_CORE_PACKED_PACKED_MAP_HPP_

#include <memoria/core/packed/map/packed_map_vle_value_base.hpp>
#include <memoria/core/packed/map/packed_map_fse_value_base.hpp>

namespace memoria {


template <
    Int Blocks_,
    typename Key_,
    typename Value_,

    typename HiddenLabels_ = TypeList<>,
    typename Labels_ = TypeList<>
>
struct PackedMapTypes {
    using Key               = Key_;
    using Value             = Value_;
    using HiddenLabels      = HiddenLabels_;
    using Labels            = Labels_;

    static const Int Blocks = Blocks_;
};



template <typename Types>
class PackedMap:
        public PackedMapValueBase<
            Types::Blocks,
            typename Types::Key,
            typename Types::Value,
            typename Types::HiddenLabels,
            typename Types::Labels
        > {

    typedef PackedMapValueBase<
            Types::Blocks,
            typename Types::Key,
            typename Types::Value,
            typename Types::HiddenLabels,
            typename Types::Labels
        >                                                                       Base;
public:
    typedef PackedMap<Types>                                                    MyType;

    typedef typename Base::Key                                                  Key;
    typedef typename Base::Value                                                Value;

    typedef typename Base::Values                                               Values;
    typedef typename Base::Values2                                              Values2;

    typedef typename Base::IndexValue                                           IndexValue;
    typedef typename Base::ValueDescr                                           ValueDescr;

    static const Int Blocks                                                     = Types::Blocks;

    static const Int SizedIndexes                                               = 1 + Blocks + Base::LabelsIndexes;

    template <typename T>
    using MapSums = StaticVector<T, SizedIndexes>;

    static const Int Indexes                                                    = SizedIndexes - 1;
    static const Int AllocatorBlocks                                            = Blocks + Base::TotalLabels
                                                                                         + Base::HasValue;
    using InputType = Values;

    static Int empty_size()
    {
        Int allocator_size      = PackedAllocator::empty_size(AllocatorBlocks);
        Int tree_empty_size     = Base::tree_empty_size();
        Int labels_empty_size   = Base::labels_empty_size();
        Int value_empty_size    = Base::value_empty_size();

        return allocator_size + tree_empty_size + labels_empty_size + value_empty_size;
    }

    static Int block_size(Int size)
    {
        return packed_block_size(size);
    }

    Int block_size(const MyType* other) const
    {
        return packed_block_size(this->size() + other->size());
    }

    Int block_size() const {
        return static_cast<const PackedAllocator*>(this)->block_size();
    }

    static Int packed_block_size(Int size)
    {
        Int tree_block_size     = Base::tree_block_size(size);
        Int values_block_size   = Base::value_block_size(size);
        Int labels_block_size   = Base::labels_block_size(size);

        Int my_block_size   = PackedAllocator::block_size(
                                    tree_block_size + values_block_size + labels_block_size,
                                    AllocatorBlocks
                              );

        return my_block_size;
    }


public:
    static Int elements_for(Int block_size)
    {
        struct ElementsForFn {
            Int block_size(Int items_number) const
            {
                return MyType::packed_block_size(items_number);
            }

            Int max_elements(Int block_size)
            {
                return block_size;
            }
        };

        return FindTotalElementsNumber2(block_size, ElementsForFn());
    }

    void init()
    {
        init(empty_size());
    }

    void init(Int block_size)
    {
        PackedAllocator::init(block_size, AllocatorBlocks);

        Base::tree_init();
        Base::labels_init();
        Base::value_init();
    }


    template <typename Entry, typename T>
    void insert(Int idx, const Entry& entry, MapSums<T>& sums)
    {
        Int size = this->size();

        Base::insertTree(idx, entry, sums);
        Base::insertLabels(idx, entry, sums);
        Base::insertValue(idx, entry, size);
    }



//    void insertSpace(Int room_start, Int room_length)
//    {
//      Int size = this->size();
//
//      Base::insertTreeSpace(room_start, room_length);
//        Base::insertLabelsSpace(room_start, room_length);
//        Base::insertValuesSpace(room_start, room_length, size);
//    }


    void removeSpace(Int room_start, Int room_end)
    {
        remove(room_start, room_end);
    }


    void remove(Int room_start, Int room_end)
    {
        Int size = this->size();

        Base::removeValuesSpace(room_start, room_end, size);
        Base::removeLabelsSpace(room_start, room_end);
        Base::removeTreeSpace(room_start, room_end);
    }


    void splitTo(MyType* other, Int split_idx)
    {
        Int size = this->size();

        Base::splitTreeTo(other, split_idx);
        Base::splitLabelsTo(other, split_idx);
        Base::splitValuesTo(other, split_idx, size);
    }


    void mergeWith(MyType* other)
    {
        Int size        = this->size();
        Int other_size  = other->size();

        Base::mergeTreeWith(other);
        Base::mergeLabelsWith(other);
        Base::mergeValuesWith(other, size, other_size);
    }


    void reindex()
    {
        Base::reindexTree();
        Base::reindexLabels();
        Base::reindexValues();
    }


    void check() const
    {
        Base::checkTree();
        Base::checkLabels();
        Base::checkValues();
    }



    ValueDescr findGEForward(Int block, Int start, IndexValue val) const
    {
        return this->tree()->findGEForward(block, start, val);
    }

    ValueDescr findGTForward(Int block, Int start, IndexValue val) const
    {
        return this->tree()->findGTForward(block, start, val);
    }

    ValueDescr findGEBackward(Int block, Int start, IndexValue val) const
    {
        return this->tree()->findGEBackward(block, start, val);
    }

    ValueDescr findGTBackward(Int block, Int start, IndexValue val) const
    {
        return this->tree()->findGTBackward(block, start, val);
    }


    ValueDescr findForward(SearchType search_type, Int block, Int start, IndexValue val) const
    {
        if (search_type == SearchType::GT)
        {
            return findGTForward(block, start, val);
        }
        else {
            return findGEForward(block, start, val);
        }
    }

    ValueDescr findBackward(SearchType search_type, Int block, Int start, IndexValue val) const
    {
        if (search_type == SearchType::GT)
        {
            return findGTBackward(block, start, val);
        }
        else {
            return findGEBackward(block, start, val);
        }
    }



    template <Int Offset, Int Size, typename T, template <typename, Int> class AccumItem>
    void sum(AccumItem<T, Size>& accum) const
    {
    	// FIXME: implement
    }

    template <Int Offset, Int Size, typename T, template <typename, Int> class AccumItem>
    void sum(Int start, Int end, AccumItem<T, Size>& accum) const
    {
    	// FIXME: implement
    }


    template <typename T>
    void sums(MapSums<T>& sums) const
    {
        Values2 tree_sums;
        this->tree()->sums(tree_sums);

        sums.sumAt(0, tree_sums);

        Base::sumsLabels(sums);
    }

    void sums(Values& values) const
    {
        this->tree()->sums(values);
    }

    void sums(Int from, Int to, Values& values) const
    {
        this->tree()->sums(from, to, values);
    }

    template <typename T>
    void sums(Int from, Int to, MapSums<T>& sums) const
    {
        Values2 tree_sums;
        this->tree()->sums(from, to, tree_sums);

        sums.sumAt(0, tree_sums);

        Base::sumsLabels(from, to, sums);
    }




    template <typename T>
    void sums(Int idx, MapSums<T>& sums) const
    {
        Values2 tree_sums;
        this->tree()->sums(idx, tree_sums);

        sums.sumAt(0, tree_sums);

        Base::sumsLabels(idx, sums);
    }

    void sums(Int idx, Values& values) const
    {
        this->tree()->sums(idx, values);
    }

    IndexValue sum(Int block) const {
        return this->tree()->sum(block);
    }

    IndexValue sum(Int block, Int to) const {
        return this->tree()->sum(block, to);
    }

    IndexValue sum(Int block, Int from, Int to) const {
        return this->tree()->sum(block, from, to);
    }

    IndexValue sumWithoutLastElement(Int block) const {
        return this->tree()->sumWithoutLastElement(block);
    }

    void addValue(Int block, Int idx, Key value)
    {
        this->tree()->addValue(block, idx, value);
    }


    template <typename Entry>
    Entry entry(Int idx) const
    {
        Entry entry;

        this->tree()->sums(idx, entry.indexes());
        entry.value() = this->value(idx);

        Base::fillLabels(idx, entry);

        return entry;
    }


    // ============================ IO =============================================== //


    template <typename Entropy>
    void estimateEntropy(Entropy& entropy, Int start, Int end) const
    {

    }

    template <typename Lengths, typename Entry>
    static void computeDataLength(const Entry& entry, Lengths& lengths)
    {
        Base::computeTreeEntryDataLength(entry,   lengths);
        Base::computeLabelsEntryDataLength(entry, lengths);
        Base::computeValueEntryDataLength(entry,  lengths);
    }


    template <typename DataSource>
    void insert(DataSource* src, Int idx, Int size)
    {
        Int old_size    = this->size();
        auto pos        = src->getStart();

        this->insertTree(src, idx, size);

        this->insertLabels(src, pos, idx, size);
        this->insertValues(src, pos, idx, size, old_size);
    }

    template <typename DataSource>
    void update(DataSource* src, Int start, Int end)
    {
        auto pos = src->getStart();

        this->updateTree(src, start, end);

        this->updateLabels(src, pos, start, end);
        this->updateValues(src, pos, start, end);
    }

    template <typename DataTarget>
    void read(DataTarget* src, Int start, Int end) const
    {
        auto pos = src->getStart();

        this->readTree(src, start, end);

        this->readLabels(src, pos, start, end);
        this->readValues(src, pos, start, end);
    }


    // ============================ Serialization ==================================== //

    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        handler->startGroup("FSE_MAP");

        Base::generateDataEvents(handler);

        handler->endGroup();
    }
};



}

#endif
