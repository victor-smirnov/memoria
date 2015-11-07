
// Copyright Victor Smirnov 2014-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_MAPX_CONTAINER_TOOLS_HPP
#define _MEMORIA_CONTAINERS_MAPX_CONTAINER_TOOLS_HPP

#include <memoria/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/core/container/container.hpp>

#include <memoria/core/packed/map/packed_map_labels_base.hpp>

namespace memoria       {
namespace mapx          {


template <
    Int Indexes,
    typename Key,
    typename Value,
    typename HiddenLabels   = std::tuple<>,
    typename Labels         = std::tuple<>
>
class MetaMapEntry {
    StaticVector<Key, Indexes>  indexes_;
    Value                       value_;
    HiddenLabels                hidden_labels_;
    Labels                      labels_;
public:

    using HiddenLabelsType  = HiddenLabels;
    using LabelsType        = Labels;

    StaticVector<Key, Indexes>& indexes()               {return indexes_;}
    const StaticVector<Key, Indexes>& indexes() const   {return indexes_;}

    Value& value()                                      {return value_;}
    const Value& value() const                          {return value_;}

    HiddenLabels& hidden_labels()                       {return hidden_labels_;}
    const HiddenLabels& hidden_labels() const           {return hidden_labels_;}

    Labels& labels()                                    {return labels_;}
    const Labels& labels() const                        {return labels_;}

    Key& key()                                          {return indexes_[0];}
    const Key& key() const                              {return indexes_[0];}
};

template <
    Int Indexes,
    typename Key,
    typename Value,
    typename HiddenLabels,
    typename Labels
>
std::ostream& operator<<(std::ostream& out, const MetaMapEntry<Indexes, Key, Value, HiddenLabels, Labels>& entry)
{
    out<<"MetaMapEntry[";
    out<<"indxs:"<<entry.indexes()<<", ";
    out<<"value:"<<entry.value()<<", ";
    out<<"h_lbls:"<<entry.hidden_labels()<<", ";
    out<<"lbls:"<<entry.labels();
    out<<"]";

    return out;
}




template <typename List> struct LabelTypeListBuilder;

template <Int Bits, typename... Tail>
struct LabelTypeListBuilder<TypeList<LabelDescr<Bits>, Tail...>> {
    typedef typename MergeLists<
                Int,
                typename LabelTypeListBuilder<
                    TypeList<Tail...>
                >::Type
    >::Result                                                                   Type;
};


template <>
struct LabelTypeListBuilder<TypeList<>> {
    typedef TypeList<>                                                          Type;
};




template <typename List> struct LabelOffsetProc;

template <Int Bits, typename... Tail>
struct LabelOffsetProc<TypeList<LabelDescr<Bits>, Tail...>> {
    static Int offset(Int label_num)
    {
        if (label_num > 0)
        {
            return (1 << Bits) + LabelOffsetProc<TypeList<Tail...>>::offset(label_num - 1);
        }
        else {
            return 0;
        }
    }
};

template <>
struct LabelOffsetProc<TypeList<>> {
    static Int offset(Int label_num)
    {
        if (label_num > 0)
        {
            throw vapi::Exception(MA_SRC, "Invalid label number requested");
        }
        else {
            return 0;
        }
    }
};


//template <Int Indexes, typename Key_, typename Value_>
//struct MapXStreamTF{
//    typedef Key_                                                    Key;
//    typedef Value_                                                  Value;
//
//    using LeafType = TL<TL<
//    		PkdFTree<Packed2TreeTypes<Key, Key, Indexes>>, //UByteExintCodec
//    		PackedFSEArray<PackedFSEArrayTypes<Value>>
//    >>;
//
//    static const Int LeafIndexes                                    = Indexes + 1;
//
//    typedef TL<TL<TL<IndexRange<0, Indexes>>, TL<>>>				IdxRangeList;
//};








template <typename Map, Int Indexes, typename Key, typename Value, typename HiddenLabels, typename Labels, typename Accum>
void InsertEntry(
        Map* map,
        Int idx,
        const MetaMapEntry<Indexes, Key, Value, HiddenLabels, Labels>& entry,
        Accum& sums
    )
{
    map->insert(idx, entry, sums);
}




template <typename Value, typename Stream>
Value& GetValueRef(Stream* stream, Int idx)
{
    return stream->value(idx);
}


template <typename Value, typename Stream>
Value GetValue(const Stream* stream, Int idx)
{
    return stream->value(idx);
}

template <typename Stream, typename Value>
void SetValue(Stream* stream, Int idx, const Value& value)
{
    stream->value(idx) = value;
}


template <typename Entry, typename Stream>
Entry GetEntry(const Stream* stream, Int idx)
{
    return stream->template entry<Entry>(idx);
}

template <typename Stream, typename Entry>
void SetEntry(const Stream* stream, Int idx, const Entry& entry)
{
    // Process entry keys here

    stream->value(idx) = entry.value();

    // Process entry labels here
}

template <typename IndexType, typename Stream>
IndexType GetLeafIndex(const Stream* stream, Int idx, Int index_num)
{
    return stream->key(index_num, idx);
}


//template <typename IndexType, typename StreamTypes>
//IndexType GetLeafIndex(const PackedVLEMap<StreamTypes>* stream, Int idx, Int index_num)
//{
//  return stream->tree()->value(index_num, idx);
//}





}
}

#endif
