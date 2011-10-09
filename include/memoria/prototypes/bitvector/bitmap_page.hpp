
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_BITMAP_PAGE_HPP
#define _MEMORIA_CORE_TOOLS_BITMAP_PAGE_HPP



#include <memoria/core/tools/bitmap.hpp>
#include <memoria/core/bitvector/btree.hpp>

#include <iostream>

namespace memoria        {
namespace bitmap_page   {



struct CountData {
    csize_t count;
    bool inserted;
    bool hasnt_label;

    CountData(){}
    CountData(csize_t _count, bool _inserted, bool _hasnt_label):
        count(_count), inserted(_inserted), hasnt_label(_hasnt_label) {}
};

template <typename Allocator, typename Header, typename Base = memoria::btree::tree_page<Allocator> >
class abstract_bitmap_page: public Base {

    Padding<sizeof(Base)>   _p0;

    Header _header;

    Padding<sizeof(Header)> _p1;

public:
    typedef Buffer<Allocator::PAGE_SIZE - sizeof(Base) - sizeof(Padding<sizeof(Base)>) - sizeof (Header) - sizeof(Padding<sizeof(Header)>)> bits_t;
private:

    bits_t _bits;

public:

    abstract_bitmap_page() {}

    bits_t &bits() {
        return _bits;
    }

    Header &header() {
        return _header;
    }
};

template <long PageSize, long BlockSize>
class bitmap_page_header {

    static const int PAGE_SIZE_SZ               = SIMPLE_LG2_32(PageSize);

    static const int NBLOCKS                    = PageSize / BlockSize;

    static const int FIELD_SIZE_SZ              = PAGE_SIZE_SZ;
    static const int FIELD_SIZE_OFFS            = 0;

    static const int FIELD_RANK_SZ              = PAGE_SIZE_SZ;
    static const int FIELD_RANK_OFFS            = FIELD_SIZE_OFFS + FIELD_SIZE_SZ;

    static const int FIELD_BITSIZE_SZ           = PAGE_SIZE_SZ;
    static const int FIELD_BITSIZE_OFFS         = FIELD_RANK_OFFS + FIELD_RANK_SZ;

    static const int FIELD_INSERTED_SZ          = PAGE_SIZE_SZ + 1;
    static const int FIELD_INSERTED_OFFS        = FIELD_BITSIZE_OFFS + FIELD_BITSIZE_SZ;

    static const int FIELD_HASNT_LABEL_SZ       = 1;
    static const int FIELD_HASNT_LABEL_OFFS     = FIELD_INSERTED_OFFS + FIELD_INSERTED_SZ;

    static const int FIELD_OFFSET_SZ            = PAGE_SIZE_SZ + 1;
    static const int FIELD_OFFSET_OFFS          = FIELD_HASNT_LABEL_OFFS + FIELD_HASNT_LABEL_SZ;

    static const int BLOCKS_BASE                = FIELD_OFFSET_OFFS + FIELD_OFFSET_SZ;
    static const int BLOCK_SIZE_SZ              = SIMPLE_LG2_32(BlockSize) + 1;

    static const int FIELD_BLOCK_RANK_SZ        = BLOCK_SIZE_SZ;
    static const int FIELD_BLOCK_RANK_OFFS      = 0;

    static const int FIELD_BLOCK_BITSIZE_SZ     = BLOCK_SIZE_SZ;
    static const int FIELD_BLOCK_BITSIZE_OFFS   = FIELD_BLOCK_RANK_SZ;

    static const int FIELD_BLOCK_OFFSET_SZ      = BLOCK_SIZE_SZ;
    static const int FIELD_BLOCK_OFFSET_OFFS    = FIELD_BLOCK_BITSIZE_OFFS + FIELD_BLOCK_BITSIZE_SZ;

    static const int FIELD_BLOCK_SZ             = FIELD_BLOCK_RANK_SZ + FIELD_BLOCK_BITSIZE_SZ + FIELD_BLOCK_OFFSET_SZ;

    static const int HEADER_BITSIZE             = FIELD_SIZE_SZ + FIELD_RANK_SZ + FIELD_BITSIZE_SZ +
                                                    FIELD_OFFSET_SZ + FIELD_BLOCK_SZ * NBLOCKS;

    static const int HEADER_SIZE                = HEADER_BITSIZE % 64 == 0 ? HEADER_BITSIZE / 8 : (HEADER_BITSIZE  + 64 - (HEADER_BITSIZE % 64)) / 8;

    Buffer<HEADER_SIZE> _buffer;

public:

    static const int BLOCKS = NBLOCKS;

    typedef typename Buffer<HEADER_SIZE>::index_t index_t;

    bitmap_page_header(){}

    index_t get_size() {
        return get_bits(_buffer, FIELD_SIZE_OFFS, FIELD_SIZE_SZ);
    }

    void set_size(index_t size) {
        set_bits(_buffer, FIELD_SIZE_OFFS, size, FIELD_SIZE_SZ);
    }

    index_t get_rank() {
        return get_bits(_buffer, FIELD_RANK_OFFS, FIELD_RANK_SZ);
    }

    void set_rank(index_t rank) {
        set_bits(_buffer, FIELD_RANK_OFFS, rank, FIELD_RANK_SZ);
    }

    index_t get_bitsize() {
        return get_bits(_buffer, FIELD_BITSIZE_OFFS, FIELD_BITSIZE_SZ);
    }

    void set_bitsize(index_t bitsize) {
        set_bits(_buffer, FIELD_BITSIZE_OFFS, bitsize, FIELD_BITSIZE_SZ);
    }

    index_t get_inserted() {
        index_t inserted = get_bits(_buffer, FIELD_INSERTED_OFFS, FIELD_INSERTED_SZ);
        return inserted >= PageSize ? -1 : inserted;
    }

    void set_inserted(index_t offset) {
        set_bits(_buffer, FIELD_INSERTED_OFFS, offset == -1 ? PageSize : offset, FIELD_INSERTED_SZ);
    }

    bool hasnt_label() {
        return get_bit(_buffer, FIELD_HASNT_LABEL_OFFS);
    }

    void set_hasnt_label(bool hasnt_label) {
        set_bit(_buffer, FIELD_HASNT_LABEL_OFFS, hasnt_label);
    }

    index_t get_offset() {
        return get_bits(_buffer, FIELD_OFFSET_OFFS, FIELD_OFFSET_SZ);
    }

    void set_offset(index_t offset) {
        set_bits(_buffer, FIELD_OFFSET_OFFS, offset, FIELD_OFFSET_SZ);
    }

    index_t get_max_blocks() {
        return NBLOCKS;
    }


    index_t get_blocks() {
        return get_size() / BlockSize + ((get_size() % BlockSize == 0) ? 0 : 1);
    }

    index_t get_rank(index_t block) {
        return get_bits(_buffer,
            BLOCKS_BASE + FIELD_BLOCK_SZ * block + FIELD_BLOCK_RANK_OFFS,
            FIELD_BLOCK_RANK_SZ
        );
    }

    void set_rank(index_t block, index_t value) {
        set_bits(_buffer,
            BLOCKS_BASE + FIELD_BLOCK_SZ * block + FIELD_BLOCK_RANK_OFFS,
            value,
            FIELD_BLOCK_RANK_SZ
        );
    }

    index_t get_bitsize(index_t block) {
        return get_bits(_buffer,
            BLOCKS_BASE + FIELD_BLOCK_SZ * block + FIELD_BLOCK_BITSIZE_OFFS,
            FIELD_BLOCK_BITSIZE_SZ
        );
    }

    void set_bitsize(index_t block, index_t value) {
        set_bits(_buffer,
            BLOCKS_BASE + FIELD_BLOCK_SZ * block + FIELD_BLOCK_BITSIZE_OFFS,
            value,
            FIELD_BLOCK_BITSIZE_SZ
        );
    }

    index_t get_offset(index_t block) {
        return get_bits(_buffer,
            BLOCKS_BASE + FIELD_BLOCK_SZ * block + FIELD_BLOCK_OFFSET_OFFS,
            FIELD_BLOCK_OFFSET_SZ
        );
    }

    void set_offset(index_t block, index_t value) {
        set_bits(_buffer,
            BLOCKS_BASE + FIELD_BLOCK_SZ * block + FIELD_BLOCK_OFFSET_OFFS,
            value,
            FIELD_BLOCK_OFFSET_SZ
        );
    }

    static index_t get_block_size() {
        return BlockSize;
    }

    static index_t get_block(index_t idx) {
        return (idx / BlockSize);
    }

    static index_t get_block_start(index_t idx) {
        return idx * BlockSize;
    }

    index_t get_block_end(index_t idx) {
        index_t end = (idx + 1) * BlockSize;
        return end <= get_size() ? end : get_size();
    }
};

template <typename Allocator, long BitBlockSize = 512>
class bitmap_page: public abstract_bitmap_page<Allocator, bitmap_page_header<Allocator::PAGE_SIZE*8, BitBlockSize> > {

public:
    typedef abstract_bitmap_page<Allocator, bitmap_page_header<Allocator::PAGE_SIZE*8, BitBlockSize> >  base;
    typedef bitmap_page<Allocator, BitBlockSize>                                                      me;
    typedef bitmap_page_header<Allocator::PAGE_SIZE*8, BitBlockSize>                                  head;

    bitmap_page(): base() {
        base::header().set_inserted(-1);
        base::set_bitmap(false);
        base::set_root(false);
        base::header().set_size(0);
        base::header().set_offset(0);
        base::header().set_rank(0);
        base::header().set_bitsize(0);
    }

    void shift(index_t from, index_t count) {
        shift_bits(base::bits(), from, from + count, base::header().get_size() - from);
        base::header().set_size(base::header().get_size() + count);
    }

    void reindex(csize_t &prefix, csize_t &offset, index_t node_bits, index_t data_bits) {
        reindex(false, false, prefix, offset, node_bits, data_bits);
    }

    void reindex(bool inserted, bool hasnt_label, csize_t &prefix, csize_t &offset,
                 index_t node_bits, index_t data_bits) {
        index_t rank, bitsize, total_rank = 0, total_bitsize = 0;
        offset = base::header().get_offset();

        index_t idx;
        for (idx = 0; idx < offset / BitBlockSize; idx++) {
            base::header().set_rank(idx,    0);
            base::header().set_bitsize(idx, 0);
            base::header().set_offset(idx,  BitBlockSize);
        }

        if (idx < get_blocks()) {
            base::header().set_offset(idx,  offset - base::header().get_block_start(idx));
        }

        for (; idx < get_blocks(); idx++) {
            index_t limit = base::header().get_block_end(idx);
            compute_rank_and_size_up_to(inserted, hasnt_label, prefix, offset, rank, bitsize, limit, node_bits, data_bits);
            inserted = hasnt_label = false;

            base::header().set_rank(idx, rank);
            base::header().set_bitsize(idx, bitsize);

            if (idx < get_blocks() - 1) {
                base::header().set_offset(idx + 1, offset - base::header().get_block_start(idx+1) < BitBlockSize ? offset - base::header().get_block_start(idx+1) : BitBlockSize);
            }

            total_rank += rank;
            total_bitsize += bitsize;
        }

        base::header().set_rank(total_rank);
        base::header().set_bitsize(total_bitsize);
    }

    index_t get_blocks() {
        return base::header().get_size() % BitBlockSize == 0 ? base::header().get_size() / BitBlockSize : base::header().get_size() / BitBlockSize + 1;
    }

    index_t get_block_size(index_t c) {
        if ((c + 1) * BitBlockSize <= base::header().get_size()) {
            return BitBlockSize;
        }
        else {
            return base::header().get_size() - c * BitBlockSize;
        }
    }

    static index_t get_max_size() {
        return sizeof(typename base::bits_t) * 8;
    }

    index_t get_capacity() {
        return get_max_size() - base::header().get_size();
    }

    void dump(std::ostream &os) {
        os<<"size        = "<<base::header().get_size()<<std::endl;
        os<<"rank        = "<<base::header().get_rank()<<std::endl;
        os<<"bitsize     = "<<base::header().get_bitsize()<<std::endl;
        os<<"offset      = "<<base::header().get_offset()<<std::endl;
        os<<"inserted    = "<<base::header().get_inserted()<<std::endl;
        os<<"hasnt_label = "<<base::header().hasnt_label()<<std::endl;

        for (index_t c = 0; c < get_blocks(); c++) {
            os<<c<<":("<<base::header().get_rank(c)<<", "<<base::header().get_bitsize(c)<<", "<<base::header().get_offset(c)<<") ";
        }
        os<<std::endl;

        base::bits().dump(os, base::header().get_size());
    }

    csize_t move_data(me &to, CountData &data, index_t from_idx,
                   index_t node_bits, index_t data_bits) {
        return move_data(to, data.count, data.inserted, data.hasnt_label, from_idx, base::header().get_size(), node_bits, data_bits);
    }

    csize_t move_data(me &to, CountData &data, index_t from_idx, index_t limit,
                   index_t node_bits, index_t data_bits) {
        return move_data(to, data.count, data.inserted, data.hasnt_label, from_idx, limit, node_bits, data_bits);
    }

    csize_t move_data(me &to, csize_t prefix,
                   bool &inserted, bool &hasnt_label, index_t from_idx,
                   index_t node_bits, index_t data_bits) {

        return move_data(to, prefix, inserted, hasnt_label, from_idx,
                         base::header().get_size(), node_bits, data_bits);
    }

    csize_t move_data(me &to, csize_t prefix,
                   bool &inserted, bool &hasnt_label, index_t from_idx, index_t limit,
                   index_t node_bits, index_t data_bits) {

        index_t from_usage = limit;//base::header().get_size();
        index_t to_usage = to.header().get_size();

        index_t length = from_usage - from_idx;

        csize_t offset = base::header().get_offset();

        csize_t base_prefix = prefix;

        bool inserted0 = inserted, hasnt_label0 = hasnt_label;
        index_t rank, bitsize;

        if (from_idx > 0) {
            csize_t prefix0 = prefix;
            compute_rank_and_size_up_to(inserted, hasnt_label, prefix, offset, rank, bitsize, from_idx, node_bits, data_bits);
            if (prefix !=  prefix0 + from_idx) {
                inserted = hasnt_label = false;
            }
        }

        csize_t offset0 = offset - from_idx;

        compute_rank_and_size_up_to(inserted, hasnt_label, prefix, offset, rank, bitsize, from_usage, node_bits, data_bits);

        index_t to_offset = to.header().get_offset();

        move(to, from_idx, from_usage, 0);

        to_usage += length;
        from_usage -= length;

        if (from_usage > length && to_offset > 0) {
            if (offset0 < length) {
                to.header().set_offset(offset0 < to_usage ? offset0 : to_usage);
            }
            else {
                to.header().set_offset(to_offset + length < to_usage ? to_offset + length : to_usage);
            }
        }
        else {
            to.header().set_offset(offset0 < to_usage ? offset0 : to_usage);
        }

        if (base::header().get_offset() > from_usage) {
            base::header().set_offset(from_usage);
        }

        base::header().set_size(from_usage);
        to.header().set_size(to_usage);

        csize_t prefix0 = base_prefix;

        offset = base::header().get_offset();

        reindex(inserted0, hasnt_label0, base_prefix, offset, node_bits, data_bits);

        index_t inserted_idx = base::header().get_inserted();

        if (base_prefix != prefix0 + from_usage) {
            if (inserted_idx >= from_usage - base_prefix) {
                inserted0 = true;
                hasnt_label0 = base::header().hasnt_label();
            }
            else {
                inserted0 = hasnt_label0 = false;
            }
        }
        else if (inserted_idx >= 0) {
            inserted0 = true;
            hasnt_label0 = base::header().hasnt_label();
        }

        offset -= from_usage;

        to.reindex(inserted0, hasnt_label0, base_prefix, offset, node_bits, data_bits);

        inserted = inserted0;
        hasnt_label = hasnt_label0;
        return base_prefix;
    }

    index_t get_offset_for(index_t limit, CountData &prefix, index_t node_bits, index_t data_bits) {
        index_t rank, bitsize;
        csize_t offset = base::header().get_offset();
        compute_rank_and_size_up_to(prefix.inserted, prefix.hasnt_label, prefix.count,
                                    offset, rank, bitsize, limit, node_bits, data_bits);
        return offset;
    }

    void compute_rank_and_size_up_to(bool inserted, bool hasnt_label, csize_t &prefix,
                                     csize_t &offset, index_t &rank, index_t &bitsize,
                                     index_t limit, index_t node_bits, index_t data_bits) {
        index_t rank1 = 0, rank0 = 0;

        index_t inserted_idx = base::header().get_inserted();
        bool hasnt_label0 = base::header().hasnt_label();

        while (offset < limit) {
            index_t sum = memoria::count_one_fw(base::bits(), offset, (csize_t)limit);
            if (sum + prefix > 0) {
                rank1 += sum;
                if (offset + sum + 1 <= limit) {
                    ++rank0;
                    offset += sum + 2 + (sum + prefix - compute_shift(offset, sum, inserted, hasnt_label, inserted_idx, hasnt_label0)) * (node_bits + ((sum + prefix > 1) ? 1 : 0) * data_bits);
                    prefix = 0;
                }
                else {
                    offset = limit;
                    prefix += sum;
                }
            }
            else {
                sum = memoria::count_zero_fw(base::bits(), offset, (csize_t)limit);
                rank0 += sum;
                prefix = 0;
                offset += sum;
            }
            hasnt_label = inserted = false;
        }

        rank = rank1;
        bitsize = rank1 + rank0;
    }

private:
    index_t compute_shift(index_t offset, index_t sum, bool inserted, bool hasnt_label, index_t inserted_idx, bool hasnt_label_local) {
        if (inserted) {
            return hasnt_label;
        }
        else if (inserted_idx >= offset && inserted_idx < offset + sum) {
            return hasnt_label_local;
        }
        else {
            return 0;
        }
    }

    void move(bitmap_page &page, index_t from, index_t limit, index_t to_start) {
        index_t count = limit - from;
        if (page.header().get_size() > 0) {
            page.shift(to_start, count);
        }
        memoria::copy_bits(base::bits(), page.bits(), from, (index_t)0, count);
    }
};


template <typename Buffer>
void compute_rank_and_size_up_to(const Buffer &buffer,
                                 index_t &prefix, index_t &offset,
                                 index_t &rank, index_t &bitsize,
                                 index_t limit, index_t node_bits, index_t data_bits) {
    index_t rank1 = 0, rank0 = 0;

    while (offset < limit) {
        index_t sum = memoria::count_one_fw(buffer, offset, (index_t)limit);
        if (sum + prefix > 0) {
            rank1 += sum;
            if (offset + sum + 1 <= limit) {
                ++rank0;
                offset += sum + 2 + (sum + prefix) * (node_bits + ((sum + prefix > 1) ? 1 : 0) * data_bits);
                prefix = 0;
            }
            else {
                offset = limit;
                prefix += sum;
            }
        }
        else {
            sum = memoria::count_zero_fw(buffer, offset, limit);
            rank0 += sum;
            prefix = 0;
            offset += sum;
        }
    }

    rank = rank1;
    bitsize = rank1 + rank0;
}

} //bitmap_page
} //memoria

#endif
