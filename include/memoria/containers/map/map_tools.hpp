
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINERS_MAP_CONTAINER_TOOLS_HPP
#define _MEMORIA_CONTAINERS_MAP_CONTAINER_TOOLS_HPP

#include <memoria/prototypes/bt/bt_tools.hpp>
#include <memoria/core/tools/static_array.hpp>
#include <memoria/core/container/container.hpp>

#include <memoria/core/packed/map/packed_fse_map.hpp>
#include <memoria/core/packed/map/packed_vle_map.hpp>

#include <memoria/core/tools/elias_codec.hpp>

namespace memoria       {
namespace map           {

template <typename Iterator, typename Container>
class MapIteratorPrefixCache: public bt::BTreeIteratorCache<Iterator, Container> {
    typedef bt::BTreeIteratorCache<Iterator, Container> Base;
    typedef typename Container::Accumulator     Accumulator;

    BigInt prefix_ = 0;
    BigInt current_ = 0;

    static const Int Indexes = 1;

public:

    MapIteratorPrefixCache(): Base(), prefix_(), current_() {}

    const BigInt& prefix(int num = 0) const
    {
        return prefix_;
    }

    const BigInt& current() const
    {
        return current_;
    }

    const Accumulator& prefixes() const
    {
        return prefix_;
    }

    void nextKey(bool end)
    {
        prefix_ += current_;

        Clear(current_);
    };

    void prevKey(bool start)
    {
        prefix_ -= current_;

        Clear(current_);
    };

    void Prepare()
    {
        if (Base::iterator().key_idx() >= 0)
        {
            current_ = Base::iterator().getRawKey();
        }
        else {
            Clear(current_);
        }
    }

    void setup(BigInt prefix)
    {
        prefix_ = prefix;

        init_(0);
    }

    void Clear(BigInt& v) const {
        v = 0;
    }

    void initState()
    {
        Clear(prefix_);

//        Int idx  = Base::iterator().key_idx();
//
//        if (idx >= 0)
//        {
//            typedef typename Iterator::Container::TreePath TreePath;
//            const TreePath& path = Base::iterator().path();
//
//            for (Int c = 0; c < path.getSize(); c++)
//            {
//                Base::iterator().model().sumKeys(path[c].node(), 0, idx, prefix_);
//                idx = path[c].parent_idx();
//            }
//        }
    }

private:

    void init_(Int skip_num)
    {
    }

};




template <typename Types, Int StreamIdx>
struct PackedFSEMapTF {

    typedef typename Types::Key                                                 Key;
    typedef typename Types::Value                                               Value;

    typedef typename SelectByIndexTool<
            StreamIdx,
            typename Types::StreamDescriptors
    >::Result                                                                   Descriptor;

    typedef PackedFSEMapTypes<
            Key, Value, Descriptor::NodeIndexes
    >                                                                           MapTypes;

    typedef PackedFSEMap<MapTypes>                                              Type;
};

template <typename Key, Granularity Gr, Int Indexes> struct CompressedMapTF;


template <typename Key, Int Indexes>
struct CompressedMapTF<Key, Granularity::Bit, Indexes> {

	typedef core::StaticVector<BigInt, Indexes>									AccumulatorPart;
	typedef core::StaticVector<BigInt, 1>										IteratorPrefixPart;

	typedef PkdFTree<Packed2TreeTypes<Key, Key, 1>> 							NonLeafType;

    typedef PackedVLEMapTypes<
            Indexes, UBigIntEliasCodec, PackedTreeEliasVPB
    > MapTypes;

    typedef PackedVLEMap<MapTypes>                                         		LeafType;
};


template <typename Key, Int Indexes>
struct CompressedMapTF<Key, Granularity::Byte, Indexes> {

	typedef core::StaticVector<BigInt, Indexes>									AccumulatorPart;
	typedef core::StaticVector<BigInt, 1>										IteratorPrefixPart;

	typedef PkdFTree<Packed2TreeTypes<Key, Key, 1>> 							NonLeafType;

    typedef PackedVLEMapTypes<
            Indexes, UByteExintCodec, PackedTreeExintVPB
    > MapTypes;

    typedef PackedVLEMap<MapTypes>                                              LeafType;
};



}
}

#endif
