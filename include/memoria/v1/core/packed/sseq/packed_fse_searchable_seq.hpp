
// Copyright 2013 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#pragma once

#include <memoria/v1/core/packed/tools/packed_allocator.hpp>
#include <memoria/v1/core/packed/tree/fse/packed_fse_quick_tree.hpp>
#include <memoria/v1/core/packed/tree/vle/packed_vle_dense_tree.hpp>
#include <memoria/v1/core/packed/tree/vle/packed_vle_quick_tree.hpp>


#include <memoria/v1/core/packed/sseq/sseq_fn/pkd_f_sseq_rank_fn.hpp>
#include <memoria/v1/core/packed/sseq/sseq_fn/pkd_f_sseq_reindex_fn.hpp>
#include <memoria/v1/core/packed/sseq/sseq_fn/pkd_f_sseq_select_fn.hpp>
#include <memoria/v1/core/packed/sseq/sseq_fn/pkd_f_sseq_tools_fn.hpp>

#include <memoria/v1/core/packed/buffer/packed_fse_sequence_input_buffer.hpp>

#include <memoria/v1/core/types/algo/select.hpp>
#include <memoria/v1/core/tools/static_array.hpp>

#include <memoria/v1/core/tools/i64_codec.hpp>
#include <memoria/v1/core/tools/elias_codec.hpp>
#include <memoria/v1/core/tools/exint_codec.hpp>
#include <memoria/v1/core/tools/i7_codec.hpp>

#include <ostream>

namespace memoria {
namespace v1 {




template <
    int32_t BitsPerSymbol_,
    int32_t VPB,

    typename IndexType,
    template <typename> class ReindexFnType = BitmapReindexFn,
    template <typename> class SelectFnType  = BitmapSelectFn,
    template <typename> class RankFnType    = BitmapRankFn,
    template <typename> class ToolsFnType   = BitmapToolsFn
>
struct PkdFSSeqTypes {

    static const int32_t Blocks                 = 1 << BitsPerSymbol_;
    static const int32_t ValuesPerBranch        = VPB;
    static const int32_t BitsPerSymbol          = BitsPerSymbol_;

    using Index     = IndexType;

    template <typename Seq>
    using ReindexFn = ReindexFnType<Seq>;

    template <typename Seq>
    using SelectFn  = SelectFnType<Seq>;

    template <typename Seq>
    using RankFn    = RankFnType<Seq>;

    template <typename Seq>
    using ToolsFn   = ToolsFnType<Seq>;
};


template <typename Types_>
class PkdFSSeq: public PackedAllocator {

    typedef PackedAllocator                                                     Base;

public:
    static const uint32_t VERSION                                                   = 1;

    typedef Types_                                                              Types;
    typedef PkdFSSeq<Types_>                                                    MyType;

    typedef PackedAllocator                                                     Allocator;

    typedef int32_t                                                                 IndexValue;

    static constexpr PkdSearchType KeySearchType = PkdSearchType::SUM;

    static constexpr int32_t ValuesPerBranch        = Types::ValuesPerBranch;
    static constexpr int32_t Indexes                = Types::Blocks;
    static constexpr int32_t BitsPerSymbol          = Types::BitsPerSymbol;
    static constexpr int32_t AlphabetSize           = 1 << BitsPerSymbol;

    static constexpr PkdSearchType SearchType = PkdSearchType::SUM;

    enum {
        METADATA, INDEX, SYMBOLS
    };

    typedef IfThenElse<
            BitsPerSymbol == 8,
            uint8_t,
            uint64_t
    >                                                                           Value;

    typedef typename Types::Index                                               Index;

    static const int32_t IndexSizeThreshold                                         = 0;
    static const PackedSizeType SizeType                                        = PkdStructSizeType<Index>::Value;

    typedef core::StaticVector<int64_t, Indexes>                                 Values;

    typedef typename Types::template ToolsFn<MyType>                            Tools;

    using InputType     = Value;

    using InputBuffer = PkdFSESequenceInputBuffer<Types>;

    class Metadata {
        int32_t size_;
    public:
        int32_t& size()                 {return size_;}
        const int32_t& size() const     {return size_;}
    };

    using SizesT = core::StaticVector<int32_t, 1>;

public:
    PkdFSSeq() = default;

    int32_t& size() {return metadata()->size();}
    const int32_t& size() const {return metadata()->size();}

    int32_t max_size() const
    {
        int32_t values_length = Base::element_size(SYMBOLS);

        int32_t symbols = values_length * 8 / BitsPerSymbol;

        return symbols;
    }

    int32_t capacity() const
    {
        return max_size() - size();
    }

    // ====================================== Accessors ================================= //

    Metadata* metadata()
    {
        return Base::template get<Metadata>(METADATA);
    }

    const Metadata* metadata() const
    {
        return Base::template get<Metadata>(METADATA);
    }

    Index* index()
    {
        return Base::template get<Index>(INDEX);
    }

    const Index* index() const
    {
        return Base::template get<Index>(INDEX);
    }

    bool has_index() const {
        return Base::element_size(INDEX) > 0;
    }

    Value* symbols()
    {
        return Base::template get<Value>(SYMBOLS);
    }

    const Value* symbols() const
    {
        return Base::template get<Value>(SYMBOLS);
    }

    class SymbolAccessor {
        MyType& seq_;
        int32_t idx_;
    public:
        SymbolAccessor(MyType& seq, int32_t idx): seq_(seq), idx_(idx) {}

        Value operator=(Value val)
        {
            seq_.set(idx_, val);
            return val;
        }

        operator Value() const {
            return seq_.get(idx_);
        }

        Value value() const {
            return seq_.get(idx_);
        }
    };

    SymbolAccessor symbol(int32_t idx)
    {
        return SymbolAccessor(*this, idx);
    }

    int32_t value(int32_t symbol, int32_t idx) const {
        return this->symbol(idx) == symbol;
    }


    class ConstSymbolAccessor {
        const MyType& seq_;
        int32_t idx_;
    public:
        ConstSymbolAccessor(const MyType& seq, int32_t idx): seq_(seq), idx_(idx) {}

        operator Value() const {
            return seq_.get(idx_);
        }

        Value value() const {
            return seq_.get(idx_);
        }
    };

    ConstSymbolAccessor symbol(int32_t idx) const
    {
        return ConstSymbolAccessor(*this, idx);
    }


public:

    // ===================================== Allocation ================================= //

    void init()
    {
        init_bs(empty_size());
    }

    void init(int32_t block_size)
    {
        MEMORIA_V1_ASSERT(block_size, >=, empty_size());

        init_bs(empty_size());
    }

    void init_bs(int32_t block_size)
    {
        Base::init(block_size, 3);

        Metadata* meta  = Base::template allocate<Metadata>(METADATA);

        meta->size()    = 0;

        Base::setBlockType(INDEX,   PackedBlockType::ALLOCATABLE);
        Base::setBlockType(SYMBOLS, PackedBlockType::RAW_MEMORY);

        // other sections are empty at this moment
    }

    int32_t block_size() const {
        return Base::block_size();
    }

    int32_t block_size(const MyType* other) const
    {
        return packed_block_size(size() + other->size());
    }

    static int32_t packed_block_size(int32_t size)
    {
        return estimate_block_size(size, 1, 1);
    }

private:
    struct ElementsForFn {
        int32_t block_size(int32_t items_number) const {
            return MyType::estimate_block_size(items_number);
        }

        int32_t max_elements(int32_t block_size)
        {
            return block_size * 8;
        }
    };

public:
    static int32_t elements_for(int32_t block_size)
    {
        return FindTotalElementsNumber2(block_size, ElementsForFn());
    }


    static int32_t empty_size()
    {
        int32_t metadata_length = Base::roundUpBytesToAlignmentBlocks(sizeof(Metadata));
        int32_t index_length    = 0;
        int32_t values_length   = 0;
        int32_t block_size      = Base::block_size(metadata_length + index_length + values_length, 3);
        return block_size;
    }

    static int32_t estimate_block_size(int32_t size, int32_t density_hi = 1, int32_t density_lo = 1)
    {
        int32_t symbols_block_size  = Base::roundUpBitsToAlignmentBlocks(size * BitsPerSymbol);
        int32_t index_size          = PackedAllocatable::divUp(size , ValuesPerBranch);
        int32_t index_block_size    = Index::estimate_block_size(index_size, density_hi, density_lo);
        int32_t metadata_block_size = Base::roundUpBytesToAlignmentBlocks(sizeof(Metadata));

        int32_t client_area         = metadata_block_size + index_block_size + symbols_block_size;
        int32_t block_size          = Base::block_size(client_area, 3);

        return block_size;
    }

    void removeIndex()
    {
        Base::free(INDEX);
    }

    template <typename IndexSizeT>
    void createIndex(IndexSizeT&& index_size)
    {
        int32_t index_block_size = Index::block_size(index_size);
        Base::resizeBlock(INDEX, index_block_size);

        Index* index = this->index();
        index->setAllocatorOffset(this);

        index->init(index_size);
    }



    std::pair<int32_t, int32_t> density() const {
        return std::pair<int32_t, int32_t>(1,1);
    }


    // ========================================= Update ================================= //

    void reindex()
    {
        typename Types::template ReindexFn<MyType> reindex_fn;
        reindex_fn.reindex(*this);
    }

    void check() const
    {
        if (has_index())
        {
            index()->check();

            typename Types::template ReindexFn<MyType> reindex_fn;
            reindex_fn.check(*this);
        }
    }

    void set(int32_t idx, int32_t symbol)
    {
        MEMORIA_V1_ASSERT(idx , <, size());

        tools().set(symbols(), idx, symbol);
    }

    void clear()
    {
        Base::resizeBlock(SYMBOLS, 0);
        removeIndex();

        size() = 0;
    }



    void enlargeData(int32_t length)
    {
        int32_t capacity = this->capacity();

        if (length >= capacity)
        {
            int32_t new_size        = size() + length;
            int32_t new_block_size  = roundUpBitToBytes(new_size * BitsPerSymbol);
            Base::resizeBlock(SYMBOLS, new_block_size);
        }
    }

protected:
    void insertDataRoom(int32_t pos, int32_t length)
    {
        enlargeData(length);

        auto symbols = this->symbols();

        int32_t rest = size() - pos;

        tools().move(symbols, pos, (pos + length), rest);

        size() += length;
    }

    void shrinkData(int32_t length)
    {
        int32_t new_size        = size() - length;

        if (new_size >= 0)
        {
            int32_t new_block_size  = roundUpBitToBytes(new_size * BitsPerSymbol);

            Base::resizeBlock(SYMBOLS, new_block_size);
        }
    }

public:

    template <int32_t Offset, int32_t Size, typename AccessorFn, typename T2, template <typename, int32_t> class BranchNodeEntryItem>
    void _insert_b(int32_t idx, BranchNodeEntryItem<T2, Size>& accum, AccessorFn&& values)
    {
        insert(idx, values(0));
    }



    void insert(int32_t pos, int32_t symbol)
    {
        insertDataRoom(pos, 1);

        Value* symbols = this->symbols();

        tools().set(symbols, pos, symbol);

        reindex();
    }

    void remove(int32_t start, int32_t end)
    {
        int32_t& size = this->size();

        MEMORIA_V1_ASSERT(start, >=, 0);
        MEMORIA_V1_ASSERT(end, >=, 0);
        MEMORIA_V1_ASSERT(start, <=, end);

        MEMORIA_V1_ASSERT(end, <=, size);

        auto symbols = this->symbols();

        int32_t rest = size - end;

        tools().move(symbols, end, start, rest);

        shrinkData(end - start);

        size -= (end - start);

        reindex();
    }

    void removeSpace(int32_t start, int32_t end) {
        remove(start, end);
    }


    void removeSymbol(int32_t idx) {
        remove(idx, idx + 1);
    }





    void fill(int32_t start, int32_t end, std::function<Value ()> fn)
    {
        auto symbols = this->symbols();
        auto tools = this->tools();

        for (int32_t c = start; c < end; c++)
        {
            Value val = fn();
            tools.set(symbols, c, val);
        }
    }

    void insert_buffer(int32_t at, const InputBuffer* buffer, int32_t start, int32_t size)
    {
        insertDataRoom(at, size);
        tools().move(buffer->symbols(), this->symbols(), start, at, size);
        reindex();
    }


    void insert(int32_t start, int32_t length, std::function<Value ()> fn)
    {
        MEMORIA_V1_ASSERT(start, >=, 0);
        MEMORIA_V1_ASSERT(start, <=, size());

        MEMORIA_V1_ASSERT(length, >=, 0);

        insertDataRoom(start, length);
        fill(start, start + length, fn);
        reindex();
    }


    template <typename Adaptor>
    void fill_with_buf(int32_t start, int32_t length, Adaptor&& adaptor)
    {
        int32_t size = this->size();

        MEMORIA_V1_ASSERT(start, >=, 0);
        MEMORIA_V1_ASSERT(start, <=, size);
        MEMORIA_V1_ASSERT(length, >=, 0);

        insertDataRoom(start, length);

        auto symbols = this->symbols();

        int32_t total = 0;

        while (total < length)
        {
            auto buf = adaptor(length - total);

            tools().move(buf.symbols(), symbols, 0, start + total, buf.size());

            total += buf.size();
        }

        reindex();
    }


    void update(int32_t start, int32_t end, std::function<Value ()> fn)
    {
        MEMORIA_V1_ASSERT(start, >=, 0);
        MEMORIA_V1_ASSERT(start, <=, end);
        MEMORIA_V1_ASSERT(end, <=, size());

        fill(start, end, fn);
        reindex();
    }


    using ReadState = SizesT;

    void read(int32_t start, int32_t end, std::function<void (Value)> fn) const
    {
        MEMORIA_V1_ASSERT(start, >=, 0);
        MEMORIA_V1_ASSERT(start, <=, end);
        MEMORIA_V1_ASSERT(end, <=, size());

        auto symbols    = this->symbols();
        auto tools      = this->tools();

        for (int32_t c = start; c < end; c++)
        {
            fn(tools.get(symbols, c));
        }
    }



    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void _insert(int32_t idx, int32_t symbol, BranchNodeEntryItem<T, Size>& accum)
    {
        insert(idx, symbol);

        sum<Offset>(idx, accum);
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void _update(int32_t idx, int32_t symbol, BranchNodeEntryItem<T, Size>& accum)
    {
        sub<Offset>(idx, accum);

        this->symbol(idx) = symbol;

        this->reindex();

        sum<Offset>(idx, accum);
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void _remove(int32_t idx, BranchNodeEntryItem<T, Size>& accum)
    {
        sub<Offset>(idx, accum);
        remove(idx, idx + 1);
    }


    // ========================================= Node ================================== //

    template <typename TreeType>
    void transferDataTo(TreeType* other) const
    {
        other->enlargeData(this->size());

        int32_t data_size = this->element_size(SYMBOLS);

        CopyByteBuffer(symbols(), other->symbols(), data_size);

        other->reindex();
    }

    void splitTo(MyType* other, int32_t idx)
    {
        int32_t to_move     = this->size() - idx;
        int32_t other_size  = other->size();

        other->enlargeData(to_move);

        auto tools = this->tools();

        tools.move(other->symbols(), other->symbols(), 0, to_move, other_size);

        tools.move(this->symbols(), other->symbols(), idx, 0, to_move);

        other->size() += to_move;
        other->reindex();

        remove(idx, this->size());
    }

    void mergeWith(MyType* other) const
    {
        int32_t my_size     = this->size();
        int32_t other_size  = other->size();

        other->enlargeData(my_size);

        tools().move(this->symbols(), other->symbols(), 0, other_size, my_size);

        other->size() += my_size;

        other->reindex();
    }


    // ========================================= Query ================================= //

    Values sums() const
    {
        if (has_index())
        {
            auto index = this->index();
            return index->sums();
        }
        else {
            return sums(size());
        }
    }


    Values sums(int32_t to) const
    {
        if (has_index())
        {
            auto index = this->index();

            int32_t index_block = to / ValuesPerBranch;

            auto isums = index->sums(0, index_block);

            auto vsums = tools().sum(index_block * ValuesPerBranch, to);

            vsums.sumUp(isums);

            return vsums;
        }
        else
        {
            auto vsums = tools().sum(0, to);
            return vsums;
        }
    }




    Values ranks(int32_t to) const
    {
        Values vals;

        for (int32_t symbol = 0; symbol < Indexes; symbol++)
        {
            vals[symbol] = rank(to, symbol);
        }

        return vals;
    }

    Values ranks() const
    {
        return this->ranks(this->size());
    }




    Values sumsAt(int32_t idx) const
    {
        Values values;
        values[symbol(idx)] = 1;

        return values;
    }

    Values sums(int32_t from, int32_t to) const
    {
        return sums(to) - sums(from);
    }


    void sums(int32_t from, int32_t to, Values& values) const
    {
        values += sums(from, to);
    }

    void sums(Values& values) const
    {
        values += sums();
    }

    Values sum_v(int32_t from, int32_t to) const {
        return sums(from, to);
    }


    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void max(BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");

        for (int32_t block = 0; block < Indexes; block++)
        {
            accum[block + Offset] = rank(block);
        }
    }



    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void sum(BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");

        for (int32_t block = 0; block < Indexes; block++)
        {
            accum[block + Offset] += rank(block);
        }
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void sum(int32_t start, int32_t end, BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");

        for (int32_t block = 0; block < Indexes; block++)
        {
            accum[block + Offset] += rank(start, end, block);
        }
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void sum(int32_t idx, BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");

        accum[symbol(idx) + Offset] ++;
    }

    template <int32_t Offset, int32_t Size, typename T, template <typename, int32_t> class BranchNodeEntryItem>
    void sub(int32_t idx, BranchNodeEntryItem<T, Size>& accum) const
    {
        static_assert(Offset <= Size - Indexes, "Invalid balanced tree structure");

        accum[symbol(idx) + Offset]--;
    }


    template <int32_t Offset, int32_t From, int32_t To, typename T, template <typename, int32_t, int32_t> class BranchNodeEntryItem>
    void sum(int32_t start, int32_t end, BranchNodeEntryItem<T, From, To>& accum) const
    {
        for (int32_t block = 0; block < Indexes; block++)
        {
            accum[block + Offset] += rank(block, start, end);
        }
    }


    int32_t sum(int32_t symbol, int32_t start, int32_t end) const
    {
        return rank(start, end, symbol);
    }

    int32_t sum(int32_t symbol, int32_t end) const
    {
        return rank(end, symbol);
    }

    int32_t sum(int32_t symbol) const
    {
        return rank(symbol);
    }





    template <typename T>
    void _add(int32_t symbol, T& value) const
    {
        value += rank(symbol);
    }

    template <typename T>
    void _add(int32_t symbol, int32_t end, T& value) const
    {
        value += rank(end, symbol);
    }

    template <typename T>
    void _add(int32_t symbol, int32_t start, int32_t end, T& value) const
    {
        value += rank(start, end, symbol);
    }



    template <typename T>
    void _sub(int32_t symbol, T& value) const
    {
        value -= rank(symbol);
    }

    template <typename T>
    void _sub(int32_t symbol, int32_t end, T& value) const
    {
        value -= rank(end, symbol);
    }

    template <typename T>
    void _sub(int32_t symbol, int32_t start, int32_t end, T& value) const
    {
        value -= rank(start, end, symbol);
    }


    int32_t get(int32_t idx) const
    {
        if (idx >= size()) {
            int a = 0; a++;
        }

        MEMORIA_V1_ASSERT(idx , <, size());
        return tools().get(symbols(), idx);
    }

    int32_t get_values(int32_t idx) const
    {
        MEMORIA_V1_ASSERT(idx , <, size());
        return tools().get(symbols(), idx);
    }

    bool test(int32_t idx, Value symbol) const
    {
        MEMORIA_V1_ASSERT(idx , <, size());
        return tools().test(symbols(), idx, symbol);
    }

    int32_t rank(int32_t symbol) const
    {
        if (has_index())
        {
            const Index* index = this->index();
            return index->sum(symbol);
        }
        else {
            return rank(size(), symbol);
        }
    }

    int32_t rank(int32_t start, int32_t end, int32_t symbol) const
    {
        int32_t rank_start  = rank(start, symbol);
        int32_t rank_end    = rank(end, symbol);

        return rank_end - rank_start;
    }

    int32_t rank(int32_t end, int32_t symbol) const
    {
        MEMORIA_V1_ASSERT(end, <=, size());
        MEMORIA_V1_ASSERT_TRUE(end >= 0);

        MEMORIA_V1_ASSERT_TRUE(symbol >= 0 && symbol < AlphabetSize);

        if (has_index())
        {
            const Index* index = this->index();

            int32_t values_block    = (end / ValuesPerBranch);
            int32_t start           = values_block * ValuesPerBranch;

            int32_t sum = index->sum(symbol, values_block);

            typename Types::template RankFn<MyType> fn(*this);

            int32_t block_sum = fn(start, end, symbol);

            return sum + block_sum;
        }
        else {
            typename Types::template RankFn<MyType> fn(*this);

            return fn(0, end, symbol);
        }
    }


    SelectResult selectFw(int32_t start, int32_t symbol, int64_t rank) const
    {
        int32_t startrank_ = this->rank(start, symbol);
        auto result = selectFw(symbol, startrank_ + rank);

        result.rank() -= startrank_;

        return result;
    }

    SelectResult selectFw(int32_t symbol, int64_t rank) const
    {
        MEMORIA_V1_ASSERT(rank, >=, 0);
        MEMORIA_V1_ASSERT_TRUE(symbol >= 0 && symbol < AlphabetSize);

        if (has_index())
        {
            const Index* index = this->index();

            int32_t index_size = index->size();

            auto result = index->findGEForward(symbol, rank);

            if (result.idx() < index_size)
            {
                int32_t start = result.idx() * ValuesPerBranch;

                typename Types::template SelectFn<MyType> fn(*this);

                int32_t localrank_ = rank - result.prefix();

                int32_t size = this->size();

                return fn(start, size, symbol, localrank_);
            }
            else {
                return SelectResult(result.idx(), result.prefix(), false);
            }
        }
        else {
            typename Types::template SelectFn<MyType> fn(*this);
            return fn(0, size(), symbol, rank);
        }
    }

    SelectResult selectBw(int32_t start, int32_t symbol, int64_t rank) const
    {
        int32_t localrank_ = this->rank(start, symbol);

        if (localrank_ >= rank)
        {
            return selectFw(symbol, localrank_ - rank + 1);
        }
        else {
            return SelectResult(-1,localrank_,false);
        }
    }

    SelectResult selectBw(int32_t symbol, int64_t rank) const
    {
        return selectBw(size(), symbol, rank);
    }

    void dump(std::ostream& out = std::cout, bool dump_index = true) const
    {
        out << "size       = " << size() << std::endl;
        out << "max_size   = " << max_size() << std::endl;
        out << "symbols    = " << Indexes << std::endl;


        if (dump_index)
        {
            out << "Index: ";
            if (has_index())
            {
                out << std::endl;
                out << "========================================" << std::endl;
                this->index()->dump(out);
                out << "========================================" << std::endl;
            }
            else {
                out << "None" << std::endl;
            }
        }

        out << std::endl;

        out << "Data:" << std::endl;

        dumpSymbols<Value>(out, size(), BitsPerSymbol, [this](int32_t idx) -> Value {
            return this->get(idx);
        });
    }

    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        handler->startGroup("PACKED_SEQUENCE");

        handler->value("SIZE",          &size());

        int32_t max_size = this->max_size();
        handler->value("MAX_SIZE",      &max_size);

        if (has_index())
        {
            index()->generateDataEvents(handler);
        }

        handler->startGroup("DATA", size());

        handler->symbols("SYMBOLS", symbols(), size(), BitsPerSymbol);

        handler->endGroup();

        handler->endGroup();
    }

    void serialize(SerializationData& buf) const
    {
        Base::serialize(buf);

        const Metadata* meta = this->metadata();

        FieldFactory<int32_t>::serialize(buf, meta->size());

        if (has_index()) {
            index()->serialize(buf);
        }

        FieldFactory<Value>::serialize(buf, symbols(), symbol_buffer_size());
    }

    void deserialize(DeserializationData& buf)
    {
        Base::deserialize(buf);

        Metadata* meta = this->metadata();

        FieldFactory<int32_t>::deserialize(buf, meta->size());

        if (has_index()) {
            index()->deserialize(buf);
        }

        FieldFactory<Value>::deserialize(buf, symbols(), symbol_buffer_size());
    }


    Tools tools() const {
        return Tools(*this);
    }


private:
    int32_t symbol_buffer_size() const
    {
        int32_t bit_size    = this->element_size(SYMBOLS) * 8;
        int32_t byte_size   = Base::roundUpBitsToAlignmentBlocks(bit_size);

        return byte_size / sizeof(Value);
    }
};

template <typename T>
struct StructSizeProvider<PkdFSSeq<T>> {
    static const int32_t Value = PkdFSSeq<T>::Indexes;
};



template <int32_t BitsPerSymbol>
struct PkdFSSeqTF: HasType<
    IfThenElse<
                BitsPerSymbol == 1,
                PkdFSSeqTypes<
                    1,
                    1024,
                    PkdFQTreeT<int32_t, 2>,
                    BitmapReindexFn,
                    BitmapSelectFn,
                    BitmapRankFn,
                    BitmapToolsFn
                >,
                IfThenElse<
                    (BitsPerSymbol > 1 && BitsPerSymbol < 8),
                    PkdFSSeqTypes<
                        BitsPerSymbol,
                        1024,
                        PkdFQTreeT<int32_t, 1 << BitsPerSymbol>,
                        ReindexFn,
                        SeqSelectFn,
                        SeqRankFn,
                        SeqToolsFn
                    >,
                    PkdFSSeqTypes<
                        BitsPerSymbol,
                        1024,
                        PkdVDTreeT<int64_t, 1 << BitsPerSymbol, UByteI7Codec>,
                        VLEReindex8BlkFn,
                        Seq8SelectFn,
                        Seq8RankFn,
                        Seq8ToolsFn
                    >
                >
    >
> {};


}}
