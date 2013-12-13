// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CORE_PACKED_FSE_MARK_MAP_HPP_
#define MEMORIA_CORE_PACKED_FSE_MARK_MAP_HPP_


#include <memoria/core/packed/tree/packed_fse_tree.hpp>
#include <memoria/core/packed/array/packed_fse_bitmap.hpp>

#include <memoria/metadata/page.hpp>

#include <ostream>

namespace memoria {

template <typename Value>
struct MarkedValue {
    typedef MarkedValue<Value>  MyType;

    typedef Int     first_type;
    typedef Value   second_type;

    Int     first;
    Value   second;

    MarkedValue() {}
    MarkedValue(Int mark, const Value& value): first(mark), second(value) {}
    MarkedValue(const Value& value): first(0), second(value) {}


    MyType& operator=(const MyType& other)
    {
        first   = other.first;
        second  = other.second;

        return *this;
    }

    Value operator=(const Value& other)
    {
        first   = 0;
        second  = other;

        return other;
    }

    bool operator==(const MyType& other) const {
        return second == other.second;
    }

    bool operator==(const Value& other) const {
        return second == other;
    }

    operator Value() const {
        return second;
    }
};




template <
    typename Key,
    typename Value_,
    Int Blocks          = 1,
    Int BitsPerSymbol_  = 1
>
struct PackedFSEMarkableMapTypes: Packed2TreeTypes<Key, BigInt, Blocks> {
    static const Int BitsPerSymbol  = BitsPerSymbol_;

    typedef Value_ MapValue;
};

template <typename Types>
class PackedFSEMarkableMap: public PackedAllocator {

    typedef PackedAllocator                                                     Base;
public:
    typedef PackedFSEMarkableMap<Types>                                         MyType;

    enum {TREE, BITMAP, ARRAY};

    typedef PkdFTree<Types>                                                     Tree;
    typedef PackedFSEBitmap<
                PackedFSEBitmapTypes<Types::BitsPerSymbol>
    >                                                                           Bitmap;

    static const Int Indexes = Tree::Blocks;

    typedef typename Types::MapValue                                            Value;
    typedef typename Types::MapValue::second_type                               SecondValue;


    typedef StaticVector<BigInt, Indexes>                                       Values;
    typedef StaticVector<BigInt, Indexes + 1>                                   Values2;

    typedef typename Tree::IndexValue                                           IndexValue;
    typedef typename Tree::ValueDescr                                           ValueDescr;



    typedef std::pair<
            typename Tree::Values,
            Value
    >                                                                           IOValue;

    typedef IDataSource<IOValue>                                                DataSource;
    typedef IDataTarget<IOValue>                                                DataTarget;

    typedef typename Bitmap::SymbolAccessor                                     SymbolAccessor;
    typedef typename Bitmap::ConstSymbolAccessor                                ConstSymbolAccessor;

    Tree* tree() {
        return Base::template get<Tree>(TREE);
    }

    const Tree* tree() const {
        return Base::template get<Tree>(TREE);
    }


    Bitmap* bitmap() {
        return Base::template get<Bitmap>(BITMAP);
    }

    const Bitmap* bitmap() const {
        return Base::template get<Bitmap>(BITMAP);
    }

    SecondValue* values() {
        return Base::template get<SecondValue>(ARRAY);
    }

    const SecondValue* values() const {
        return Base::template get<SecondValue>(ARRAY);
    }

    Int size() const
    {
        return tree()->size();
    }


    class ValueAccessor {
        MyType& me_;
        Int idx_;
    public:
        ValueAccessor(MyType& me, Int idx): me_(me), idx_(idx) {}

        Value operator=(const Value& value)
        {
            me_.mark(idx_)      = value.first;
            me_.values()[idx_]  = value.second;
            return value;
        }

        SecondValue operator=(const SecondValue& value)
        {
            me_.values()[idx_] = value;
            return value;
        }

        operator Value() const {
            return Value(me_.mark(idx_), me_.values()[idx_]);
        }

//      operator SecondValue() const {
//          return me_.values()[idx_];
//      }
    };


    class ConstValueAccessor {
        const MyType& me_;
        Int idx_;
    public:
        ConstValueAccessor(const MyType& me, Int idx): me_(me), idx_(idx) {}

        operator Value() const {
            return Value(me_.mark(idx_), me_.values()[idx_]);
        }

//      operator SecondValue() const {
//          return me_.values()[idx_];
//      }
    };

    ValueAccessor value(Int idx)
    {
        return ValueAccessor(*this, idx);
    }

    ConstValueAccessor value(Int idx) const
    {
        return ConstValueAccessor(*this, idx);
    }


    SymbolAccessor mark(Int idx) {
        return bitmap()->symbol(idx);
    }

    const ConstSymbolAccessor mark(Int idx) const {
        return bitmap()->symbol(idx);
    }



    static Int empty_size()
    {
        Int allocator_size      = PackedAllocator::empty_size(3);
        Int tree_empty_size     = Tree::empty_size();
        Int bitmap_empty_size   = Bitmap::empty_size();

        return allocator_size + tree_empty_size + bitmap_empty_size;
    }

    static Int block_size(Int size)
    {
        return packed_block_size(size);
    }

    Int block_size(const MyType* other) const
    {
        return block_size(size() + other->size());
    }

    Int block_size() const {
        return Base::block_size();
    }

    static Int packed_block_size(Int size)
    {
        Int tree_block_size     = Tree::packed_block_size(size);
        Int bitmap_block_size   = Bitmap::packed_block_size(size);

        Int data_block_size     = Base::roundUpBytesToAlignmentBlocks(sizeof(Value) * size);

        Int my_block_size       = Base::block_size(tree_block_size + bitmap_block_size + data_block_size, 3);

        return my_block_size;
    }

private:
    struct ElementsForFn {
        Int block_size(Int items_number) const {
            return MyType::packed_block_size(items_number);
        }

        Int max_elements(Int block_size)
        {
            return block_size;
        }
    };

public:
    static Int elements_for(Int block_size)
    {
        return FindTotalElementsNumber2(block_size, ElementsForFn());
    }

    void init()
    {
        Base::init(empty_size(), 3);

        Base::template allocateEmpty<Tree>(TREE);
        Base::template allocateEmpty<Bitmap>(BITMAP);
        Base::template allocateArrayByLength<SecondValue>(ARRAY, 0);
    }

    void init(Int block_size)
    {
        Base::init(block_size, 3);

        Base::template allocateEmpty<Tree>(TREE);
        Base::template allocateEmpty<Bitmap>(BITMAP);
        Base::template allocateArrayByLength<SecondValue>(ARRAY, 0);
    }

    void insert(Int idx, const Values& keys, const Value& value)
    {
        Int size = this->size();

        tree()->insert(idx, keys);
        bitmap()->insert(idx, value.first);

        Int requested_block_size = (size + 1) * sizeof(SecondValue);

        Base::resizeBlock(ARRAY, requested_block_size);

        SecondValue* values = this->values();

        CopyBuffer(values + idx, values + idx + 1, size - idx);

        values[idx] = value.second;
    }

    void insert(Int idx, const Values2& keys2, const Value& value)
    {
        Values keys;

        for (Int c = 0; c < Values::Indexes; c++)
        {
            keys[c] = keys2[c + 1];
        }

        this->insert(idx, keys, value);
    }


    void insertSpace(Int room_start, Int room_length)
    {
        Int size = this->size();

        MEMORIA_ASSERT(room_start, >=, 0);
        MEMORIA_ASSERT(room_length, >=, 0);

        tree()->insertSpace(room_start, room_length);
        bitmap()->insertSpace(room_start, room_length);

        Int requested_block_size = (size + room_length) * sizeof(SecondValue);

        Base::resizeBlock(ARRAY, requested_block_size);

        SecondValue* values = this->values();

        CopyBuffer(values + room_start, values + room_start + room_length, size - room_start);
    }

    void removeSpace(Int room_start, Int room_end)
    {
        remove(room_start, room_end);
    }

    void remove(Int room_start, Int room_end)
    {
        Int old_size = this->size();

        MEMORIA_ASSERT(room_start, >=, 0);
        MEMORIA_ASSERT(room_end, >=, room_start);
        MEMORIA_ASSERT(room_end, <=, old_size);

        SecondValue* values = this->values();

        CopyBuffer(values + room_end, values + room_start, old_size - room_end);

        Int requested_block_size = (old_size - (room_end - room_start)) * sizeof(SecondValue);

        Base::resizeBlock(values, requested_block_size);

        tree()->remove(room_start, room_end);
        bitmap()->remove(room_start, room_end);

        tree()->reindex();
    }

    void splitTo(MyType* other, Int split_idx)
    {
        Int size = this->size();

        tree()->splitTo(other->tree(), split_idx);
        bitmap()->splitTo(other->bitmap(), split_idx);

        Int remainder = size - split_idx;

        other->template allocateArrayBySize<SecondValue>(ARRAY, remainder);

        SecondValue* other_values = other->values();
        SecondValue* my_values    = this->values();

        CopyBuffer(my_values + split_idx, other_values, remainder);
    }

    void mergeWith(MyType* other)
    {
        Int other_size  = other->size();
        Int my_size     = this->size();

        tree()->mergeWith(other->tree());
        bitmap()->mergeWith(other->bitmap());

        Int other_values_block_size          = other->element_size(ARRAY);
        Int required_other_values_block_size = (my_size + other_size) * sizeof(SecondValue);

        if (required_other_values_block_size >= other_values_block_size)
        {
            other->resizeBlock(ARRAY, required_other_values_block_size);
        }

        CopyBuffer(values(), other->values() + other_size, my_size);
    }

    void reindex() {
        tree()->reindex();
    }

    void check() const
    {
        tree()->check();
        bitmap()->check();
    }

    void dump(std::ostream& out = std::cout) const
    {
        tree()->dump(out);
        bitmap()->dump(out);

        Int size = this->size();

        auto values = this->values();

        out<<"DATA"<<std::endl;

        for (Int c = 0; c < size; c++)
        {
            out<<c<<" "<<values[c]<<std::endl;
        }

        out<<std::endl;
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
        return this->tree()->findGEForward(block, start, val);
    }

    ValueDescr findGTBackward(Int block, Int start, IndexValue val) const
    {
        return this->tree()->findGTForward(block, start, val);
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




    void sums(Values2& values) const
    {
        tree()->sums(values);
    }

    void sums(Values& values) const
    {
        tree()->sums(values);
    }

    void sums(Int from, Int to, Values& values) const
    {
        tree()->sums(from, to, values);
    }

    void sums(Int from, Int to, Values2& values) const
    {
        tree()->sums(from, to, values);
    }

    IndexValue sum(Int block) const
    {
        return tree()->sum(block);
    }

    IndexValue sum(Int block, Int to) const
    {
        return tree()->sum(block, to);
    }

    IndexValue sum(Int block, Int from, Int to) const
    {
        return tree()->sum(block, from, to);
    }

    IndexValue sumWithoutLastElement(Int block) const
    {
        return tree()->sumWithoutLastElement(block);
    }

    // ============================ IO =============================================== //


    void insert(IData* data, Int pos, Int length)
    {
        DataSource* src = static_cast<DataSource*>(data);

        MEMORIA_ASSERT_TRUE(to_bool(src->api() & IDataAPI::Single));
        MEMORIA_ASSERT(src->getRemainder(), >=, length);

        for (SizeT c = 0; c < length; c++)
        {
            IOValue v = src->get();

            this->insert(pos + c, v.first, v.second);
        }

        reindex();
    }

    void update(IData* data, Int pos, Int length)
    {
        MEMORIA_ASSERT(pos, <=, size());
        MEMORIA_ASSERT(pos + length, <=, size());

        DataSource* src = static_cast<DataSource*>(data);

        MEMORIA_ASSERT_TRUE(to_bool(src->api() & IDataAPI::Single));
        MEMORIA_ASSERT(src->getRemainder(), >=, length);

        Tree*   tree        = this->tree();
        Bitmap* bitmap      = this->bitmap();
        SecondValue* array  = this->values();

        for (SizeT c = pos; c < pos + length; c++)
        {
            auto v = src->get();

            for (Int d = 0; d < Values::Indexes; d++)
            {
                tree->value(d, c)   = v.first[d];
            }

            bitmap->symbol(c)   = v.second.first;
            array[c]            = v.second.second;
        }

        reindex();
    }

    void read(IData* data, Int pos, Int length) const
    {
        MEMORIA_ASSERT(pos, <=, size());
        MEMORIA_ASSERT(pos + length, <=, size());

        DataTarget* tgt = static_cast<DataTarget*>(data);

        MEMORIA_ASSERT_TRUE(to_bool(tgt->api() & IDataAPI::Single));
        MEMORIA_ASSERT(tgt->getRemainder(), >=, length);

        const Tree*  tree           = this->tree();
        const Bitmap* bitmap        = this->bitmap();
        const SecondValue* array    = this->values();

        for (SizeT c = pos; c < pos + length; c++)
        {
            IOValue v(tree->values(c), std::pair<Int, Value>(bitmap->symbol(c), array[c]));
            tgt->put(v);
        }
    }


    // ============================ Serialization ==================================== //

    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        handler->startGroup("FSE_MARKABLE_MAP");

        Base::generateDataEvents(handler);

        tree()->generateDataEvents(handler);
        bitmap()->generateDataEvents(handler);

        handler->startGroup("DATA", size());

        auto values = this->values();

        for (Int idx = 0; idx < size(); idx++)
        {
            vapi::ValueHelper<SecondValue>::setup(handler, values[idx]);
        }

        handler->endGroup();

        handler->endGroup();
    }

    void serialize(SerializationData& buf) const
    {
        Base::serialize(buf);

        tree()->serialize(buf);
        bitmap()->serialize(buf);

        FieldFactory<SecondValue>::serialize(buf, values(), size());
    }

    void deserialize(DeserializationData& buf)
    {
        Base::deserialize(buf);

        tree()->deserialize(buf);
        bitmap()->deserialize(buf);

        FieldFactory<SecondValue>::deserialize(buf, values(), size());
    }

};



}


namespace std {
template <typename Value>
ostream& operator<<(ostream& out, const memoria::MarkedValue<Value>& value) {
    out<<"["<<value.first<<", "<<value.second<<"]";
    return out;
}
}

#endif
