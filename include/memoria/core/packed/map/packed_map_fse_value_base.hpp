
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CORE_PACKED_MAP_FSE_VALUE_BASE_HPP_
#define MEMORIA_CORE_PACKED_MAP_FSE_VALUE_BASE_HPP_


#include <memoria/core/packed/map/packed_map_labels_base.hpp>


namespace memoria {



template <Int Blocks, typename Key, typename Value_, typename HiddenLabels, typename Labels>
class PackedMapValueBase: public PackedMapLabelsBase<Blocks, Key, HiddenLabels, Labels> {

    typedef PackedMapLabelsBase<Blocks, Key, HiddenLabels, Labels>              Base;
public:
    typedef PackedMapValueBase<Blocks, Key, Value_, HiddenLabels, Labels>       MyType;

    typedef Value_                                                              Value;


    static const Int TotalLabels 	= ListSize<HiddenLabels>::Value + ListSize<Labels>::Value;
    static const Int ARRAY 			= Blocks + TotalLabels;

    static const bool HasValue                                                  = true;

    Value* values() {
        return Base::template get<Value>(ARRAY);
    }

    const Value* values() const {
        return Base::template get<Value>(ARRAY);
    }

    Value& value(Int idx)
    {
        return values()[idx];
    }

    const Value& value(Int idx) const
    {
        return values()[idx];
    }

    static Int value_empty_size()
    {
        return 0;
    }

    static Int value_block_size(Int size)
    {
        Int block_size = Base::roundUpBytesToAlignmentBlocks(sizeof(Value)*size);
        return block_size;
    }


    void value_init()
    {
        Base::template allocateArrayByLength<Value>(ARRAY, 0);
    }

    template <typename Entry>
    void insertValue(Int idx, const Entry& entry, Int size)
    {
        Int requested_block_size = (size + 1) * sizeof(Value);

        Base::resizeBlock(ARRAY, requested_block_size);

        Value* values = this->values();

        CopyBuffer(values + idx, values + idx + 1, size - idx);

        values[idx] = entry.value();
    }


    void insertValuesSpace(Int room_start, Int room_length, Int size)
    {
        Int requested_block_size = (size + room_length) * sizeof(Value);

        Base::resizeBlock(ARRAY, requested_block_size);

        Value* values = this->values();

        CopyBuffer(values + room_start, values + room_start + room_length, size - room_start);
    }


    void removeValuesSpace(Int room_start, Int room_end, Int old_size)
    {
        Value* values = this->values();

        CopyBuffer(values + room_end, values + room_start, old_size - room_end);

        Int requested_block_size = (old_size - (room_end - room_start)) * sizeof(Value);

        Base::resizeBlock(values, requested_block_size);
    }


    void splitValuesTo(MyType* other, Int split_idx, Int size)
    {
        Int remainder = size - split_idx;

        other->template allocateArrayBySize<Value>(ARRAY, remainder);

        Value* other_values = other->values();
        Value* my_values    = this->values();

        CopyBuffer(my_values + split_idx, other_values, remainder);
    }



    void mergeValuesWith(MyType* other, Int my_size, Int other_size)
    {
        Int other_values_block_size          = other->element_size(ARRAY);
        Int required_other_values_block_size = (my_size + other_size) * sizeof(Value);

        if (required_other_values_block_size >= other_values_block_size)
        {
            other->resizeBlock(ARRAY, required_other_values_block_size);
        }

        CopyBuffer(values(), other->values() + other_size, my_size);
    }


    void reindexValues()
    {
    }

    void checkValues() const
    {}


    void dumpValues(std::ostream& out = std::cout) const
    {
        out<<"VALUES"<<std::endl;

        auto values = this->values();

        Int size = this->size();

        for (Int c = 0; c < size; c++)
        {
            out<<c<<" "<<values[c]<<std::endl;
        }

        out<<std::endl;
    }


    // ============================ IO =============================================== //


    template <typename Entry, typename Lengths>
    static void computeValueEntryDataLength(const Entry& entry, Lengths& lengths)
    {
    	std::get<1 + TotalLabels>(lengths)++;
    }


    template <typename DataSource>
    void insertValues(DataSource* src, SizeT pos, Int start, Int size, Int old_size)
    {
    	insertValuesSpace(start, size, old_size);

    	updateValues(src, pos, start, start + size);
    }


    template <typename DataSource>
    void updateValues(DataSource* src, SizeT pos, Int start, Int end)
    {
    	src->reset(pos);

    	auto* values = this->values();

    	for (Int c = start; c < end; c++)
    	{
    		values[c] = src->get().value();
    	}
    }

    template <typename DataTarget>
    void readValues(DataTarget* tgt, SizeT pos, Int start, Int end) const
    {
    	tgt->reset(pos);

    	auto* values = this->values();

    	for (Int c = start; c < end; c++)
    	{
    		auto current 	= tgt->peek();
    		current.value() = values[c];

    		tgt->put(current);
    	}
    }

    // ============================ Serialization ==================================== //


    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        Base::generateDataEvents(handler);

        Int size = this->size();

        handler->startGroup("VALUES", size);

        auto values = this->values();

        for (Int idx = 0; idx < size; idx++)
        {
            vapi::ValueHelper<Value>::setup(handler, values[idx]);
        }

        handler->endGroup();
    }


    void serialize(SerializationData& buf) const
    {
        Base::serialize(buf);
        FieldFactory<Value>::serialize(buf, values(), this->size());
    }

    void deserialize(DeserializationData& buf)
    {
        Base::deserialize(buf);
        FieldFactory<Value>::deserialize(buf, values(), this->size());
    }
};



}

#endif
