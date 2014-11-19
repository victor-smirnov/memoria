// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CORE_PACKED_MAP_VLE_VALUE_BASE_HPP_
#define MEMORIA_CORE_PACKED_MAP_VLE_VALUE_BASE_HPP_


#include <memoria/core/packed/map/packed_map_labels_base.hpp>
#include <memoria/core/packed/array/packed_vle_array.hpp>
#include <memoria/core/packed/tree/packed_tree_tools.hpp>


namespace memoria {

namespace internal {

template <Granularity gr, typename ValueType> struct VLEArrayTF;

template <typename ValueType>
struct VLEArrayTF<Granularity::Bit, ValueType> {
    typedef PackedVLEArray<
            Packed2TreeTypes<
                ValueType, typename PackedTreeIndexTF<ValueType>::Type, 1, UBigIntEliasCodec
            >
    >                                                                           Type;
};

template <typename ValueType>
struct VLEArrayTF<Granularity::Byte, ValueType> {
    typedef PackedVLEArray<
            Packed2TreeTypes<
                ValueType, typename PackedTreeIndexTF<ValueType>::Type, 1, UByteExintCodec
            >
    >                                                                           Type;
};


}



template <
    Int Blocks,
    typename Key,
    typename ValueType,
    typename HiddenLabels,
    typename Labels
> class PackedMapValueBase;

template <
    Int Blocks,
    typename Key,
    Granularity gr,
    typename ValueType,
    typename
    HiddenLabels,
    typename Labels
>
class PackedMapValueBase<Blocks, Key, VLen<gr, ValueType>, HiddenLabels, Labels>:
        public PackedMapLabelsBase<Blocks, Key, HiddenLabels, Labels> {

    typedef PackedMapLabelsBase<Blocks, Key, HiddenLabels, Labels>                      Base;
public:
    typedef PackedMapValueBase<Blocks, Key, VLen<gr, ValueType>, HiddenLabels, Labels>  MyType;

    typedef typename memoria::internal::VLEArrayTF<gr, ValueType>::Type                 Values;
    typedef typename Values::Values                                                     VValues;
    typedef ValueType                                                                   Value;

    static const Int TotalLabels = ListSize<HiddenLabels>::Value + ListSize<Labels>::Value;

    static const Int ARRAY = Blocks + TotalLabels;

    static const bool HasValue                                                          = true;

    Values* values() {
        return Base::template get<Values>(ARRAY);
    }

    const Values* values() const {
        return Base::template get<Values>(ARRAY);
    }

    typename Values::ValueAccessor value(Int idx)
    {
        return values()->value(0, idx);
    }

    const Value value(Int idx) const
    {
        return values()->getValue(0, idx);
    }

    static Int value_empty_size()
    {
        return Values::empty_size();
    }

    static Int value_block_size(Int size)
    {
        Int block_size = Values::estimate_block_size(size);
        return block_size;
    }


    void value_init()
    {
        PackedAllocator::template allocateEmpty<Values>(ARRAY);
    }

    template <typename Entry>
    void insertValue(Int idx, const Entry& entry, Int)
    {
        values()->insert(idx, entry.value());
    }


    void insertValueSpace(Int room_start, Int room_length, Int)
    {
        values()->insertSpace(room_start, room_length);
    }


    void removeValuesSpace(Int room_start, Int room_end, Int)
    {
        values()->removeSpace(room_start, room_end);
    }


    void splitValuesTo(MyType* other, Int split_idx, Int size)
    {
        values()->splitTo(other->values(), split_idx);
    }

    void mergeValuesWith(MyType* other, Int my_size, Int other_size)
    {
        values()->mergeWith(other->values());
    }


    void reindexValues()
    {
        values()->reindex();
    }

    void checkValues() const
    {
        values()->check();
    }


    void dumpValues(std::ostream& out = std::cout) const
    {
        out<<"VALUES"<<std::endl;

        auto values = this->values();

        Int size = this->size();

        for (Int c = 0; c < size; c++)
        {
            out<<c<<" "<<values->value(c)<<std::endl;
        }

        out<<std::endl;
    }



    // ============================ IO =============================================== //


    template <typename Entry, typename Lengths>
    static void computeValueEntryDataLength(const Entry& entry, Lengths& lengths)
    {
        std::get<1 + TotalLabels>(lengths) += Values::computeDataLength(VValues(entry.value()));
    }


    template <typename DataSource>
    void insertValues(DataSource* src, SizeT pos, Int start, Int size, Int old_size)
    {
        src->reset(pos);

        values()->insert(start, size, [src]() {
            return VValues(src->get().value());
        });
    }


    template <typename DataSource>
    void updateValues(DataSource* src, SizeT pos, Int start, Int end)
    {
        src->reset(pos);

        values()->update(start, end, [src]() {
            return VValues(src->get().value());
        });
    }

    template <typename DataTarget>
    void readValues(DataTarget* tgt, SizeT pos, Int start, Int end) const
    {
        tgt->reset(pos);

        values()->read(start, end, [=](const typename Values::Values& value) {
            auto current    = tgt->peek();
            current.value() = value.get();

            tgt->put(current);
        });
    }


    // ============================ Serialization ==================================== //


    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        Base::generateDataEvents(handler);

        handler->startGroup("VALUES", this->size());

        values()->generateDataEvents(handler);

        handler->endGroup();
    }


    void serialize(SerializationData& buf) const
    {
        Base::serialize(buf);
        values()->serialize(buf);
    }


    void deserialize(DeserializationData& buf)
    {
        Base::deserialize(buf);
        values()->deserialize(buf);
    }
};



}

#endif
