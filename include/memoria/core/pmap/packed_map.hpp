
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_PACKED_MAP_H
#define _MEMORIA_CORE_TOOLS_PACKED_MAP_H
//dummy
#include <memoria/core/tools/buffer.hpp>
#include <memoria/core/types/typehash.hpp>
#include <memoria/core/pmap/packed_map_intrnl.hpp>

#include <memoria/core/tools/reflection.hpp>

#include <iostream>
#include <typeinfo>

namespace memoria        {

using memoria::vapi::MetadataList;

template <
	size_t BlockSize,
	Int kind = 0,
	Int Children = 16
>
struct PackedMapMetadata {
	static const size_t BLOCK_SIZE = BlockSize;
	static const Int KIND = kind;
	static const Int CHILDREN = Children;
};

template <
	typename Key_,
	typename Value_,
	typename IndexKey_,
	typename Constants_,
	long Indexes_,
	template <typename> class Accumulator_,
	template <typename> class CmpLT_,
	template <typename> class CmpLE_,
	template <typename> class CmpEQ_
>
struct PackedMapTypesBase {

	typedef Key_ 						Key;
	typedef Value_ 						Value;
	typedef IndexKey_ 					IndexKey;
	typedef Constants_ 					Constants;
	typedef Accumulator_<IndexKey>		Accumulator;
	typedef CmpLT_<IndexKey>			CmpLT;
	typedef CmpLE_<IndexKey>			CmpLE;
	typedef CmpEQ_<IndexKey>			CmpEQ;
	static const long Indexes			= Indexes_;
};

template <typename Types>
class PackedMap: public Buffer<Types::Constants::BLOCK_SIZE> {

	typedef typename Types::Key 				Key;
	typedef typename Types::Value 				Value;
	typedef typename Types::Constants 			Constants;
	static const long 							Indexes 			= Types::Indexes;
	typedef typename Types::IndexKey 			IndexKey;
	typedef typename Types::Accumulator 		Accumulator;
	typedef typename Types::CmpLT 				CmpLT;
	typedef typename Types::CmpLE 				CmpLE;
	typedef typename Types::CmpEQ 				CmpEQ;

public:

    typedef PackedMap<Types>																	Me;

    static const long INDEXES   = Indexes;

    static const Int kHashCode  = ListHash<
                        typename TLTool<
                            Key,
                            Value,
                            HashCode<Constants::BLOCK_SIZE>,
                            HashCode<Indexes>,
                            IndexKey
                        >::List
    >::Value;


private:
    typedef Buffer<Constants::BLOCK_SIZE> 														Base;

    //4 * Constants::CACHE_LINE_WIDTH / (sizeof(IndexKey) * Indexes);
    static const Int children0		= Constants::CHILDREN;

    static const Int children       = children0 > 1 ? children0 : 2;

    static const Int size_offset    = 0;                //all offsets are in bytes
    static const Int index_offset   = sizeof(Int);

    static const size_t value_size = ValueTraits<Value>::Size;

    static Int      index_size_;
    static Int      data_offset_;
    static Int      max_size_;

    static bool     initialized;

public:

    PackedMap(): Base () {
        initialized = true;
    }

    MetadataList GetFields(Long &abi_ptr) const
    {
        MetadataList list;

        FieldFactory<Int>::create(list, size(), "SIZE", abi_ptr);

        MetadataList indexList;
        for (Int c = 0; c < index_size(); c++)
        {
        	FieldFactory<IndexKey>::create(indexList, index(c), "INDEX", Indexes, abi_ptr);
        }
        list.push_back(new MetadataGroupImpl("INDEXES", indexList));

        MetadataList dataList;
        for (Int c = 0; c < max_size(); c++)
        {
            MetadataList itemList;
            FieldFactory<Key>::create(itemList, key(c), "KEYS", Indexes, abi_ptr);

            if (value_size > 0) {
                FieldFactory<Value>::create(itemList, data(c), "DATA", abi_ptr);
            }

            dataList.push_back(new MetadataGroupImpl("ITEM", itemList));
        }
        list.push_back(new MetadataGroupImpl("ITEMS", dataList));

        return list;
    }

    static int kind() {
        return Constants::KIND;
    }

    static Int block_size() {
        return Constants::BLOCK_SIZE;
    }

    const Int &size() const {
        return P2CR<Int>(Base::ptr() + size_offset);
    }

    Int &size() {
        return P2R<Int>(Base::ptr() + size_offset);
    }

    static Int index_size() {
        return index_size_;
    }

    static Int max_size() {
        return max_size_;
    }

    const IndexKey &index(Int n) const {
        return index(0, n);
    }

    IndexKey &index(Int n) {
        return index(0, n);
    }

    const IndexKey &index(Int i, Int n) const {
        Int idx = index_offset + n * sizeof(IndexKey)*Indexes + sizeof(IndexKey) * i;
        return P2CR<IndexKey>(Base::ptr() + idx);
    }

    IndexKey &index(Int i, Int n) {
        Int idx = index_offset + n * sizeof(IndexKey)*Indexes + sizeof(IndexKey) * i;
        return P2R<IndexKey>(Base::ptr() + idx);
    }

    const Key &key(Int n) const {
        return key(0, n);
    }

    Key &key(Int n) {
        return key(0, n);
    }

    const Key &key(Int i, Int n) const {
        Int idx = data_offset_ + n * (sizeof(Key)*Indexes + value_size) + sizeof(Key) * i;
        return P2CR<Key>(Base::ptr() + idx);
    }

    Key &key(Int i, Int n) {
        Int idx = data_offset_ + n * (sizeof(Key)*Indexes + value_size) + sizeof(Key) * i;
        return P2R<Key>(Base::ptr() + idx);
    }


    const Key* keys(Int n) const {
        Int idx = data_offset_ + n * (sizeof(Key)*Indexes + value_size);
        return T2T<const Key*>(Base::ptr()  + idx);
    }

    Key* keys(Int n) {
        Int idx = data_offset_ + n * (sizeof(Key)*Indexes + value_size);
        return T2T<Key*>(Base::ptr()  + idx);
    }
    
    const Value &data(Int n) const {
        Int idx = data_offset_ + n * (sizeof(Key)*Indexes + value_size) + sizeof(Key)*Indexes;
        return P2CR<Value>(Base::ptr() + idx);
    }

    Value &data(Int n) {
        Int idx = data_offset_ + n * (sizeof(Key)*Indexes + value_size) + sizeof(Key)*Indexes;
        return P2R<Value>(Base::ptr() + idx);
    }

    Int Add(const Key &k, const Value &value) {
        set(size(), k, value);
        size()++;
        return size();
    }

    Int Add(const Key& k) {
        key(size()) = k;
        size()++;
        return size();
    }

    void set(Int idx, const Key& k, const Value &value) {
        set(0, idx, k, value);
    }

    void set(Int i, Int idx, const Key& k, const Value &value) {
        key(i, idx) = k;
        data(idx) = value;
    }

    const IndexKey& max_key() const {
        return max_key(0);
    }

    const IndexKey& max_key(Int i) const {
        return index(i, 0);
    }

    const Key& min_key() const {
        return min_key(0);
    }

    const Key& min_key(Int i) const {
        return key(i, 0);
    }

    void Reindex() {
    	for (Int c = 0; c < Indexes; c++) {
            Reindex(c);
        }
    }

    void Reindex(Int index_num) {
        Int _size = size();
        if (_size > 0)
        {
            Int isize = get_index_size(_size);
            Int size0 = get_parent_nodes_for(_size);

            Accumulator acc;
            Int idx;
            for (idx = 0; idx < (_size - _size % children); idx += children) {

                for (Int i = idx; i < idx + children; i++) {
                    acc(key(index_num, i));
                }

                index(index_num, isize - size0 + (idx / children)) = acc.get();
                acc.Reset();
            }

            if ((_size % children) != 0) {
                for (Int i = idx; i < _size; i++) {
                    acc(key(index_num, i));
                }

                index(index_num, isize - size0 + (idx / children)) = acc.get();
            }

            Int i_base = isize - size0;
            _size = size0;

            while (size0 > 1) {
                size0 = get_parent_nodes_for(_size);
                Accumulator acc1;

                for (idx = 0; idx < (_size - _size % children); idx += children) {
                    for (Int i = idx + i_base; i < idx + children + i_base; i++) {
                        acc1(index(index_num, i));
                    }
                    
                    index(index_num, i_base - size0 + (idx / children)) = acc1.get();
                    acc1.Reset();
                }

                if ((_size % children) != 0) {
                    for (Int i = idx + i_base; i < _size + i_base; i++) {
                        acc1(index(index_num, i));
                    }

                    index(index_num, i_base - size0 + (idx / children)) = acc1.get();
                }

                i_base -= size0;
                _size = size0;
            }
        }
    }

    template <typename IndexComparator, typename DataComparator>
    Int Find(Int index_num, const Key& k, IndexComparator &index_comparator, DataComparator &data_comparator) const
    {
        if (data_comparator.TestMax(k, max_key(index_num)))
        {
            return -1;
        }

        Int levels = get_levels_for_size(size());
        Int base = 0, start = 0;

        for (Int level = 0; level < levels; level++)
        {
            Int size = get_elements_on_level(level);

            for (Int idx = start; idx < size; idx++)
            {
                const Key& key0 = index(index_num, base + idx);
                if (index_comparator(k, key0)) {
                    start = (idx * children);
                    index_comparator.Sub(key0);
                    break;
                }
            }

            base += size;
        }

        if (data_comparator.isShouldReset())
        {
        	data_comparator.Reset(index_comparator.get());
        }

        Int _size = size();
        Int stop = (start + children) > _size ? _size : start + children;
        for (Int idx = start; idx < stop; idx++)
        {
            //Key key0 = key(index_num, idx);
            if (data_comparator(k, key(index_num, idx))) {
                return idx;
            }
        }

        return -1;
    }

    Int FindLT(const Key& k) const {
        return FindLT(0, k);
    }

    Int FindLT(Int i, const Key& k) const {
        CmpLT lt0, lt1;
        return Find(i, k, lt0, lt1);
    }

    Int FindLTS(const Key& k, Key &sum) const {
        return FindLTS(0, k, sum);
    }

    Int FindLTS(Int i, const Key& k, Key &sum) const {
        CmpLT lt0, lt1;
        Int idx = Find(i, k, lt0, lt1);
        sum += lt1.get() - (size() > 0 ? key(i, idx) : 0);
        return idx;
    }

    Int FindLE(const Key& k) const {
        return FindLE(0, k);
    }

    Int FindLE(Int i, const Key& k) const {
        CmpLE le0, le1;
        return Find(i, k, le0, le1);
    }

    Int FindLES(const Key& k, Key &sum) const {
        return FindLES(0, k, sum);
    }

    Int FindLES(Int i, const Key& k, Key &sum) const {
        CmpLE le0, le1;
        Int idx = Find(i, k, le0, le1);
        sum += le1.get() - (size() > 0 ? key(i, idx) : 0);
        return idx;
    }

    Int FindEQ(const Key& k) const {
        return FindEQ(0, k);
    }

    Int FindEQ(Int i, const Key& k) const {
        CmpLE le;
        CmpEQ eq;
        return Find(i, k, le, eq);
    }

    Key Prefix(Int i, Int idx, const Key& sum) const {
        return sum - key(i, idx);
    }

    void Dump(std::ostream &os) const {
        Dump(0, os);
    }

    void Dump(Int i, std::ostream &os) const {

        Int _size = size();
        os<<"------------------------------------------"<<std::endl;
        os<<"Key: "<<TypeNameFactory<Key>::name()<<" IndexKey: "<<TypeNameFactory<IndexKey>::name()<<std::endl;
        os<<"levels: ";
        Int nlevels = get_levels_for_size(_size);
        for (Int c = 0, count = 0; c < nlevels; c++) {
            count = get_elements_on_level(c);
            os<<count<<" ";
        }
        os<<std::endl;

        os<<"size: "<<size()<<std::endl;
        os<<"index:"<<std::endl;

        Int isize = get_index_size(_size);
        for (Int c = 0; c < isize; c++) {
            os<<c<<":"<<index(i, c)<<" ";
        }

        os<<std::endl;
        os<<"data:"<<std::endl;
        for (Int c = 0; c < _size; c++) {
            os<<c<<":"<<key(i, c)<<" ";
        }
        os<<std::endl;
    }

    void MoveData(Int dest, Int src, Int count, const Key& k, const Value &value) {
        Int c;
        if (src < dest) {
            --src;
            --dest;
            for(c = count; c > 0 ; c--) {
                key(dest + c) = key(src + c);
                key(src + c) = k;
                if (value_size > 0) {
                    data(dest + c) = data(src + c);
                    data(src + c)  = value;
                }
            }
        }
        else {
            for(c = 0; c < count ; c++) {
                key(dest + c) = key(src + c);
                key(src + c) = k;
                if (value_size > 0) {
                    data(dest + c) = data(src + c);
                    data(src + c)  = value;
                }
            }
        }
    }

    void MoveData(Int dest, Int src, Int count) {
        Int c;
        if (src < dest) {
            --src;
            --dest;
            for(c = count; c > 0 ; c--) {
                for (Int j = 0; j < Indexes; j++) {
                    key(j, dest + c) = key(j, src + c);
                }

                if (value_size > 0) {
                    data(dest + c) = data(src + c);
                }
            }
        }
        else {
            for(c = 0; c < count ; c++) {
                for (Int j = 0; j < Indexes; j++) {
                    key(j, dest + c) = key(j, src + c);
                }

                if (value_size > 0) {
                    data(dest + c) = data(src + c);
                }
            }
        }
    }

    void CopyData(Int from, Int count, Me &other, Int to) const {
        for(Int c = from; c < from + count ; c++) {
            for (Int j = 0; j < Indexes; j++) {
                other.key(j, to + c - from) = key(j, c);
            }

            if (value_size > 0) {
                other.data(to + c - from) = data(c);
            }
        }
    }

//    void MoveData(Int dest, Int src, Int count, const Key& k) {
//        MoveData(dest, src, Value());
//    }

    //TODO: needs testing
    void Remove(Int idx, Int count) {
        const Int size0 = size();
        MoveData(idx, idx + count, size0 - idx - count);
        size() -= count;

        for (Int c = size(); c < size0; c++) {
            key(c) = 0;
        }
    }

    Int get_index_levels_count() const {
        return get_levels_for_size(size());
    }

    Int get_elements_on_level(Int level) const {
        Int _size = size();
        Int levels = get_levels_for_size(_size);

        for (Int nlevels = levels - 1; nlevels >= level; nlevels--) {
            _size = ((_size % children) == 0) ? (_size / children) : (_size / children) + 1;
        }

        return _size;
    }

    void MoveTo(Me &target, Int from) {
        Int count = size() - from;

        if (target.size() > 0) {
            target.MoveData(count, 0, target.size());
        }

        for (Int i = from; i < size(); i++) {
            target.key(i - from) = key(i);
            if (value_size > 0) {
                target.data(i - from) = data(i);
            }
        }

        Reindex();
        target.Reindex();
    }

protected:

    static Int get_index_size(Int csize) {
        if (csize == 1) {
            return 1;
        }
        else {
            Int sum = 0;
            for (Int nlevels=0; csize > 1; nlevels++) {
                csize = ((csize % children) == 0) ? (csize / children) : (csize / children) + 1;
                sum += csize;
            }
            return sum;
        }
    }

    static Int get_levels_for_size(Int csize) {
        Int nlevels;
        for (nlevels=0; csize > 1; nlevels++) {
            csize = ((csize % children) == 0) ? (csize / children) : (csize / children) + 1;
        }
        return nlevels;
    }

    static Int get_parent_nodes_for(Int n) {
        return (n % children) == 0 ? n / children : ((n / children) + 1);
    }

    static Int get_block_size(Int item_count) {
        Int key_size = sizeof(IndexKey) * Indexes;
        Int item_size = sizeof(Key)*Indexes + value_size;
        return get_index_size(item_count) * key_size + item_count * item_size;
    }
public:
    static bool Init();
};



template <typename Types>
Int PackedMap<Types>::data_offset_    = 0;

template   <typename Types>
Int PackedMap<Types>::index_size_     = 0;

template   <typename Types>
Int PackedMap<Types>::max_size_       = 0;


template   <typename Types>
bool PackedMap<Types>::Init() {
    Int block_size = Constants::BLOCK_SIZE - sizeof(Int);
    Int item_size = sizeof(Key)*Indexes + value_size;
    
    Int first = 1;
    Int last  = block_size / item_size;
    while (first < last - 1) {
        Int middle = (first + last) / 2;

        Int size = get_block_size(middle);
        if (size < block_size) {
            first = middle;
        }
        else if (size > block_size) {
            last = middle;
        }
        else {
            break;
        }
    }

    if (get_block_size(last) <= block_size) {
        max_size_ = last;
    }
    else if (get_block_size((first + last) / 2) <= block_size) {
        max_size_ = (first + last) / 2;
    }
    else {
        max_size_ = first;
    }

    index_size_ = get_index_size(max_size_);

    data_offset_ = sizeof(Int) + index_size_ * sizeof(IndexKey) * Indexes;

    //cout<<"Init: index_size="<<index_size_<<" children="<<children<<" soik="<<sizeof(IndexKey)<<endl;

    //if (data_offset_ % 4 != 0) data_offset_ += (data_offset_ % 4);

//    cout<<"data_offset: "<<data_offset_<<" "<<(data_offset_%4)<<endl;

    initialized = true;

    return true;
}

template   <typename Types>
bool PackedMap<Types>::initialized = PackedMap<Types>::Init();


template <typename Types>
struct PackedValueMapTypes: public PackedMapTypesBase <
	typename Types::Key,
	typename Types::Value,
	typename Types::IndexKey,
	PackedMapMetadata<Types::BlockSize, 0, Types::Children>,
	Types::Indexes,
	Tracker,
	CompareLT,
	CompareLE,
	CompareEQ
> {};

template <typename Types>
struct PackedIndexMapTypes: public PackedMapTypesBase <
	typename Types::Key,
	typename Types::Value,
	typename Types::IndexKey,
	PackedMapMetadata<Types::BlockSize, 1, Types::Children>,
	Types::Indexes,
	Accumulator,
	CompareLTAcc,
	CompareLEAcc,
	CompareEQAcc
> {};


template <typename Types, int index> struct PackedMapTypes;

template <typename Types>
struct PackedMapTypes<Types, MapTypes::Index>: public PackedIndexMapTypes<Types> {};

template <typename Types>
struct PackedMapTypes<Types, MapTypes::Value>: public PackedValueMapTypes<Types> {};




} //memoria

#endif
