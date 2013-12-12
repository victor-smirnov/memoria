
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CORE_PACKED_MAP_EMPTY_VALUE_BASE_HPP_
#define MEMORIA_CORE_PACKED_MAP_EMPTY_VALUE_BASE_HPP_


#include <memoria/core/packed/map/packed_map_labels_base.hpp>


namespace memoria {

template <typename Types> class PackedMapValueBase;

template <Int Blocks, typename Key, typename Value, typename HiddenLabels, typename Labels>
class PackedMapValueBase<
	Blocks,
	Key,
	EmptyType,
	HiddenLabels,
	Labels
>: public PackedMapLabelsBase<Blocks, Key, HiddenLabels, Labels> {

    typedef PackedMapLabelsBase<Blocks, Key, HiddenLabels, Labels>              Base;
public:
    typedef PackedMapValueBase<Blocks, Key, EmptyType, HiddenLabels, Labels>    MyType;

    typedef EmptyType															Value;


    static const bool HasValue													= false;



    static Int value_empty_size()
    {
        return 0;
    }

    static Int value_block_size(Int size)
    {
    	return 0;
    }


    void value_init(){}

    void insertValue(Int idx, const Value& value) {}

    void insertValueSpace(Int room_start, Int room_length) {}

    void removeValueSpace(Int room_start, Int room_end) {}

    void splitValuesTo(MyType* other, Int split_idx, Int size) {}

    void mergeValuesWith(MyType* other, Int my_size, Int other_size) {}

    void reindexValue() {}

    void checkValue() const {}

    void dumpValues(std::ostream& out = std::cout) const {}


    // ============================ Serialization ==================================== //


    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        Base::generateDataEvents(handler);
    }


    void serialize(SerializationData& buf) const
    {
        Base::serialize(buf);
    }

    void deserialize(DeserializationData& buf)
    {
        Base::deserialize(buf);
    }
};



}

#endif
