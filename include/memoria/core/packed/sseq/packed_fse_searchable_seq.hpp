
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

#include <memoria/core/packed/tools/packed_allocator.hpp>
#include <memoria/core/packed/tree/fse/packed_fse_quick_tree.hpp>
#include <memoria/core/packed/tree/vle/packed_vle_dense_tree.hpp>
#include <memoria/core/packed/tree/vle/packed_vle_quick_tree.hpp>


#include <memoria/core/packed/sseq/sseq_fn/pkd_f_sseq_rank_fn.hpp>
#include <memoria/core/packed/sseq/sseq_fn/pkd_f_sseq_reindex_fn.hpp>
#include <memoria/core/packed/sseq/sseq_fn/pkd_f_sseq_select_fn.hpp>
#include <memoria/core/packed/sseq/sseq_fn/pkd_f_sseq_tools_fn.hpp>

#include <memoria/core/types/algo/select.hpp>
#include <memoria/core/tools/static_array.hpp>

#include <memoria/core/tools/i64_codec.hpp>
#include <memoria/core/tools/elias_codec.hpp>
#include <memoria/core/tools/exint_codec.hpp>
#include <memoria/core/tools/i7_codec.hpp>

#include <ostream>

namespace memoria {

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

    static const int32_t IndexSizeThreshold                                     = 0;
    static const PackedDataTypeSize SizeType = PkdStructSizeType<Index>;

    typedef core::StaticVector<int64_t, Indexes>                                Values;

    typedef typename Types::template ToolsFn<MyType>                            Tools;

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

    VoidResult init() noexcept
    {
        return init_bs(empty_size());
    }

    VoidResult init(int32_t block_size) noexcept
    {
        MEMORIA_ASSERT_RTN(block_size, >=, empty_size());

        return init_bs(empty_size());
    }

    VoidResult init_bs(int32_t block_size) noexcept
    {
        MEMORIA_TRY_VOID(Base::init(block_size, 3));

        MEMORIA_TRY(meta, Base::template allocate<Metadata>(METADATA));
        meta->size() = 0;

        Base::setBlockType(INDEX,   PackedBlockType::ALLOCATABLE);
        Base::setBlockType(SYMBOLS, PackedBlockType::RAW_MEMORY);

        // other sections are empty at this moment
        return VoidResult::of();
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


    static int32_t empty_size() noexcept
    {
        int32_t metadata_length = PackedAllocatable::roundUpBytesToAlignmentBlocks(sizeof(Metadata));
        int32_t index_length    = 0;
        int32_t values_length   = 0;
        int32_t block_size      = Base::block_size(metadata_length + index_length + values_length, 3);
        return block_size;
    }

    static int32_t estimate_block_size(int32_t size, int32_t density_hi = 1, int32_t density_lo = 1) noexcept
    {
        int32_t symbols_block_size  = PackedAllocatable::roundUpBitsToAlignmentBlocks(size * BitsPerSymbol);
        int32_t index_size          = PackedAllocatable::divUp(size , ValuesPerBranch);
        int32_t index_block_size    = Index::estimate_block_size(index_size, density_hi, density_lo);
        int32_t metadata_block_size = PackedAllocatable::roundUpBytesToAlignmentBlocks(sizeof(Metadata));

        int32_t client_area         = metadata_block_size + index_block_size + symbols_block_size;
        int32_t block_size          = Base::block_size(client_area, 3);

        return block_size;
    }

    VoidResult removeIndex() noexcept
    {
        MEMORIA_TRY_VOID(Base::free(INDEX));
        return VoidResult::of();
    }

    template <typename IndexSizeT>
    VoidResult createIndex(IndexSizeT&& index_size) noexcept
    {
        int32_t index_block_size = Index::block_size(index_size);
        MEMORIA_TRY_VOID(Base::resizeBlock(INDEX, index_block_size));

        Index* index = this->index();
        index->allocatable().setAllocatorOffset(this);

        MEMORIA_TRY_VOID(index->init(index_size));
        return VoidResult::of();
    }



    std::pair<int32_t, int32_t> density() const {
        return std::pair<int32_t, int32_t>(1,1);
    }


    // ========================================= Update ================================= //

    VoidResult reindex() noexcept
    {
        typename Types::template ReindexFn<MyType> reindex_fn;
        return reindex_fn.reindex(*this);
    }

    VoidResult check() const noexcept
    {
        return wrap_throwing([&]() -> VoidResult {
            if (has_index())
            {
                MEMORIA_TRY_VOID(index()->check());

                typename Types::template ReindexFn<MyType> reindex_fn;
                return reindex_fn.check(*this);
            }
            return VoidResult::of();
        });
    }

    void set(int32_t idx, int32_t symbol)
    {
        MEMORIA_ASSERT(idx , <, size());

        tools().set(symbols(), idx, symbol);
    }

    VoidResult clear() noexcept
    {
        MEMORIA_TRY_VOID(Base::resizeBlock(SYMBOLS, 0));

        MEMORIA_TRY_VOID(removeIndex());

        size() = 0;

        return VoidResult::of();
    }



    VoidResult enlargeData(int32_t length) noexcept
    {
        int32_t capacity = this->capacity();

        if (length >= capacity)
        {
            int32_t new_size        = size() + length;
            int32_t new_block_size  = PackedAllocatable::roundUpBitToBytes(new_size * BitsPerSymbol);
            MEMORIA_TRY_VOID(Base::resizeBlock(SYMBOLS, new_block_size));
        }

        return VoidResult::of();
    }

    template <typename AccessorFn>
    VoidResult insert_entries(psize_t row_at, psize_t size, AccessorFn&& elements) noexcept
    {
        MEMORIA_ASSERT_RTN(row_at, <=, this->size());

        MEMORIA_TRY_VOID(insertDataRoom(row_at, size));

        for (psize_t c = 0; c < size; c++) {
            set(c + row_at, elements(c));
        }

        return VoidResult::of();
    }

    template <typename AccessorFn>
    VoidResult update_entries(psize_t row_at, psize_t size, AccessorFn&& elements) noexcept
    {
        MEMORIA_TRY_VOID(remove_entries(row_at, size));
        return insert_entries(row_at, size, std::forward<AccessorFn>(elements));
    }

    template <typename AccessorFn>
    VoidResult remove_entries(psize_t row_at, psize_t size) noexcept
    {
        MEMORIA_ASSERT_RTN(row_at + size, <=, this->size());
        return removeSpace(row_at, row_at + size);
    }


protected:
    VoidResult insertDataRoom(int32_t pos, int32_t length) noexcept
    {
        MEMORIA_TRY_VOID(enlargeData(length));

        auto symbols = this->symbols();

        int32_t rest = size() - pos;

        tools().move(symbols, pos, (pos + length), rest);

        size() += length;

        return VoidResult::of();
    }

    VoidResult shrinkData(int32_t length) noexcept
    {
        int32_t new_size = size() - length;

        if (new_size >= 0)
        {
            int32_t new_block_size = PackedAllocatable::roundUpBitToBytes(new_size * BitsPerSymbol);
            MEMORIA_TRY_VOID(Base::resizeBlock(SYMBOLS, new_block_size));
        }

        return VoidResult::of();
    }

public:

    VoidResult insert(int32_t pos, int32_t symbol) noexcept
    {
        MEMORIA_TRY_VOID(insertDataRoom(pos, 1));

        Value* symbols = this->symbols();

        tools().set(symbols, pos, symbol);

        return reindex();
    }

    VoidResult remove(int32_t start, int32_t end) noexcept
    {
        int32_t& size = this->size();

        MEMORIA_ASSERT_RTN(start, >=, 0);
        MEMORIA_ASSERT_RTN(end, >=, 0);
        MEMORIA_ASSERT_RTN(start, <=, end);

        MEMORIA_ASSERT_RTN(end, <=, size);

        auto symbols = this->symbols();

        int32_t rest = size - end;

        tools().move(symbols, end, start, rest);

        MEMORIA_TRY_VOID(shrinkData(end - start));

        size -= (end - start);

        return reindex();
    }

    VoidResult removeSpace(int32_t start, int32_t end) noexcept {
        return remove(start, end);
    }


    VoidResult removeSymbol(int32_t idx) noexcept {
        return remove(idx, idx + 1);
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


    VoidResult insert(int32_t start, int32_t length, std::function<Value ()> fn) noexcept
    {
        MEMORIA_ASSERT_RTN(start, >=, 0);
        MEMORIA_ASSERT_RTN(start, <=, size());

        MEMORIA_ASSERT_RTN(length, >=, 0);

        MEMORIA_TRY_VOID(insertDataRoom(start, length));

        fill(start, start + length, fn);

        return reindex();
    }


    template <typename Adaptor>
    VoidResult fill_with_buf(int32_t start, int32_t length, Adaptor&& adaptor) noexcept
    {
        int32_t size = this->size();

        MEMORIA_ASSERT_RTN(start, >=, 0);
        MEMORIA_ASSERT_RTN(start, <=, size);
        MEMORIA_ASSERT_RTN(length, >=, 0);

        MEMORIA_TRY_VOID(insertDataRoom(start, length));

        auto symbols = this->symbols();

        int32_t total = 0;

        while (total < length)
        {
            auto buf = adaptor(length - total);

            tools().move(buf.symbols(), symbols, 0, start + total, buf.size());

            total += buf.size();
        }

        return reindex();
    }


    VoidResult update(int32_t start, int32_t end, std::function<Value ()> fn) noexcept
    {
        MEMORIA_ASSERT_RTN(start, >=, 0);
        MEMORIA_ASSERT_RTN(start, <=, end);
        MEMORIA_ASSERT_RTN(end, <=, size());

        fill(start, end, fn);
        return reindex();
    }


    using ReadState = SizesT;

    void read(int32_t start, int32_t end, std::function<void (Value)> fn) const
    {
        MEMORIA_ASSERT(start, >=, 0);
        MEMORIA_ASSERT(start, <=, end);
        MEMORIA_ASSERT(end, <=, size());

        auto symbols    = this->symbols();
        auto tools      = this->tools();

        for (int32_t c = start; c < end; c++)
        {
            fn(tools.get(symbols, c));
        }
    }



    VoidResult splitTo(MyType* other, int32_t idx) noexcept
    {
        int32_t to_move     = this->size() - idx;
        int32_t other_size  = other->size();

        MEMORIA_TRY_VOID(other->enlargeData(to_move));

        auto tools = this->tools();

        tools.move(other->symbols(), other->symbols(), 0, to_move, other_size);
        tools.move(this->symbols(), other->symbols(), idx, 0, to_move);

        other->size() += to_move;
        MEMORIA_TRY_VOID(other->reindex());

        return remove(idx, this->size());
    }

    VoidResult mergeWith(MyType* other) const noexcept
    {
        int32_t my_size     = this->size();
        int32_t other_size  = other->size();

        MEMORIA_TRY_VOID(other->enlargeData(my_size));

        tools().move(this->symbols(), other->symbols(), 0, other_size, my_size);

        other->size() += my_size;

        return other->reindex();
    }


    // ========================================= Query ================================= //

    int32_t get(int32_t idx) const
    {
        if (idx >= size()) {
            int a = 0; a++;
        }

        MEMORIA_ASSERT(idx , <, size());
        return tools().get(symbols(), idx);
    }


    bool test(int32_t idx, Value symbol) const
    {
        MEMORIA_ASSERT(idx , <, size());
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
        MEMORIA_ASSERT(end, <=, size());
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
        MEMORIA_ASSERT(rank, >=, 0);
        MEMORIA_V1_ASSERT_TRUE(symbol >= 0 && symbol < AlphabetSize);

        if (has_index())
        {
            const Index* index = this->index();

            int32_t index_size = index->size();

            auto result = index->findGEForward(symbol, rank);

            if (result.local_pos() < index_size)
            {
                int32_t start = result.local_pos() * ValuesPerBranch;

                typename Types::template SelectFn<MyType> fn(*this);

                int32_t localrank_ = rank - result.prefix();

                int32_t size = this->size();

                return fn(start, size, symbol, localrank_);
            }
            else {
                return SelectResult(result.local_pos(), result.prefix(), false);
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


    VoidResult generateDataEvents(IBlockDataEventHandler* handler) const noexcept
    {
        handler->startGroup("PACKED_SEQUENCE");

        handler->value("SIZE",          &size());

        int32_t max_size = this->max_size();
        handler->value("MAX_SIZE",      &max_size);

        if (has_index())
        {
            MEMORIA_TRY_VOID(index()->generateDataEvents(handler));
        }

        handler->startGroup("DATA", size());

        handler->symbols("SYMBOLS", symbols(), size(), BitsPerSymbol);

        handler->endGroup();

        handler->endGroup();

        return VoidResult::of();
    }

    template <typename SerializationData>
    VoidResult serialize(SerializationData& buf) const noexcept
    {
        MEMORIA_TRY_VOID(Base::serialize(buf));

        const Metadata* meta = this->metadata();

        FieldFactory<int32_t>::serialize(buf, meta->size());

        if (has_index()) {
            MEMORIA_TRY_VOID(index()->serialize(buf));
        }

        FieldFactory<Value>::serialize(buf, symbols(), symbol_buffer_size());

        return VoidResult::of();
    }

    template <typename DeserializationData>
    VoidResult deserialize(DeserializationData& buf) noexcept
    {
        MEMORIA_TRY_VOID(Base::deserialize(buf));

        Metadata* meta = this->metadata();

        FieldFactory<int32_t>::deserialize(buf, meta->size());

        if (has_index()) {
            MEMORIA_TRY_VOID(index()->deserialize(buf));
        }

        FieldFactory<Value>::deserialize(buf, symbols(), symbol_buffer_size());

        return VoidResult::of();
    }


    Tools tools() const {
        return Tools(*this);
    }


private:
    int32_t symbol_buffer_size() const
    {
        int32_t bit_size    = this->element_size(SYMBOLS) * 8;
        int32_t byte_size   = PackedAllocatable::roundUpBitsToAlignmentBlocks(bit_size);

        return byte_size / sizeof(Value);
    }
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


}
