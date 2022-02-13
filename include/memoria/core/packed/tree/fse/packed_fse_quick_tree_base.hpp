
// Copyright 2015 Victor Smirnov
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

#include <memoria/core/packed/tree/fse/packed_fse_quick_tree_base_base.hpp>

#include <memoria/core/tools/assert.hpp>
#include <memoria/core/tools/result.hpp>


namespace memoria {

template <typename IndexValueT, typename ValueT, size_t kBranchingFactor, size_t kValuesPerBranch> class PkdFQTreeBase;

class PkdFQTreeMetadata {
    psize_t size_;
    psize_t index_size_;
    psize_t max_size_;
public:
    PkdFQTreeMetadata() = default;

    size_t size() const {return size_;}
    void set_size(size_t val) {size_ = val;}

    void add_size(size_t val) {size_ += val;}
    void sub_size(size_t val) {size_ -= val;}

    size_t index_size() const {return index_size_;}
    void set_index_size(size_t val) {index_size_ = val;}

    size_t max_size() const {return max_size_;}
    void set_max_size(size_t val) {max_size_ = val;}

    size_t capacity() const {
        return max_size_ - size_;
    }

    psize_t& size_mut() {return size_;}
    psize_t& max_size_mut() {return max_size_;}
    psize_t& index_size_mut() {return index_size_;}

    const psize_t& size_imm() const {return size_;}
    const psize_t& max_size_imm() const {return max_size_;}
    const psize_t& index_size_imm() const {return index_size_;}

    template <typename, size_t, size_t, size_t, typename> friend class PkdFQTreeBaseBase;
    template <typename, typename, size_t, size_t> friend class PkdFQTreeBase;
};



template <typename IndexValueT, typename ValueT, size_t kBranchingFactor, size_t kValuesPerBranch>
class PkdFQTreeBase: public PkdFQTreeBaseBase<IndexValueT, kBranchingFactor, kValuesPerBranch, 2, PkdFQTreeMetadata> {

    using Base      = PkdFQTreeBaseBase<IndexValueT, kBranchingFactor, kValuesPerBranch, 2, PkdFQTreeMetadata>;
    using MyType    = PkdFQTreeBase<IndexValueT, ValueT, kBranchingFactor, kValuesPerBranch>;

public:
    static constexpr uint32_t VERSION = 1;

    using IndexValue    = IndexValueT;
    using Value         = ValueT;

    using Metadata      = typename Base::Metadata;
    using TreeLayout    = typename Base::template IndexedTreeLayout<IndexValue>;


    static const size_t BranchingFactor        = kBranchingFactor;
    static const size_t ValuesPerBranch        = kValuesPerBranch;

    static const bool FixedSizeElement      = true;

    static constexpr size_t ValuesPerBranchMask    = ValuesPerBranch - 1;
    static constexpr size_t BranchingFactorMask    = BranchingFactor - 1;

    static constexpr size_t ValuesPerBranchLog2    = Log2(ValuesPerBranch) - 1;
    static constexpr size_t BranchingFactorLog2    = Log2(BranchingFactor) - 1;

    static constexpr size_t SegmentsPerBlock = 2;

    static constexpr size_t METADATA = 0;


    struct InitFn {
        size_t blocks_;

        InitFn(size_t blocks): blocks_(blocks) {}

        size_t block_size(size_t items_number) const  {
            return MyType::block_size(blocks_, items_number);
        }

        size_t max_elements(size_t block_size)
        {
            return block_size;
        }
    };

public:

    PkdFQTreeBase() = default;

    using FieldsList = MergeLists<
                typename Base::FieldsList,
                decltype(Metadata::max_size_),
                ConstValue<uint32_t, VERSION>,
                Value
    >;


    VoidResult init_tl(size_t data_block_size, size_t blocks)
    {
        MEMORIA_TRY_VOID(Base::init(data_block_size, blocks * SegmentsPerBlock + 1));

        MEMORIA_TRY(meta, this->template allocate<Metadata>(METADATA));

        size_t max_size = 0;

        meta->size()        = 0;
        meta->max_size()    = max_size;
        meta->index_size()  = MyType::index_size(max_size);

        for (size_t block = 0; block < blocks; block++)
        {
            MEMORIA_TRY_VOID(this->template allocate_array_by_size<IndexValue>(block * SegmentsPerBlock + 1, meta->index_size()));
            MEMORIA_TRY_VOID(this->template allocate_array_by_size<Value>(block * SegmentsPerBlock + 2, max_size));
        }

        return VoidResult::of();
    }



    static size_t block_size(size_t blocks, size_t capacity)
    {
        size_t metadata_length = PackedAllocatable::round_up_bytes_to_alignment_blocks(sizeof(Metadata));

        size_t index_size      = MyType::index_size(capacity);
        size_t index_length    = PackedAllocatable::round_up_bytes_to_alignment_blocks(index_size * sizeof(IndexValue));

        size_t values_length   = PackedAllocatable::round_up_bytes_to_alignment_blocks(capacity * sizeof(Value));

        return Base::block_size(metadata_length + (index_length + values_length) * blocks, blocks * SegmentsPerBlock + 1);
    }

    size_t block_size() const
    {
        return Base::block_size();
    }

    static size_t index_size(size_t capacity)
    {
        TreeLayout layout;
        Base::compute_tree_layout(capacity, layout);
        return layout.index_size;
    }

    static size_t tree_size(size_t blocks, size_t block_size)
    {
        if (block_size >= (size_t)sizeof(Value)) {
            return FindTotalElementsNumber2(block_size, InitFn(blocks));
        }
        else {
            return 0;
        }
    }


    Metadata* metadata() {
        return this->template get<Metadata>(METADATA);
    }
    const Metadata* metadata() const {
        return this->template get<Metadata>(METADATA);
    }

    IndexValue* index(size_t block) {
        return Base::template index<IndexValue, 0>(block);
    }

    const IndexValue* index(size_t block) const {
        return Base::template index<IndexValue, 0>(block);
    }



    Value* values(size_t block) {
        return this->template get<Value>(block * SegmentsPerBlock + 2);
    }
    const Value* values(size_t block) const {
        return this->template get<Value>(block * SegmentsPerBlock + 2);
    }

    Value& value(size_t block, size_t idx) {
        return values(block)[idx];
    }

    const Value& value(size_t block, size_t idx) const {
        return values(block)[idx];
    }

    class WalkerBase {
    protected:
        size_t idx_;
        IndexValue sum_;
    public:
        WalkerBase(size_t idx, IndexValue sum): idx_(idx), sum_(sum) {}

        auto idx() const {return idx_;}
        auto prefix() const {return sum_;}
    };

    struct FindGEWalker: WalkerBase {
    protected:
        IndexValue target_;
        IndexValue next_{};
        using WalkerBase::idx_;
        using WalkerBase::sum_;
    public:
        using WalkerBase::idx;

        FindGEWalker(IndexValue target):
            WalkerBase(0,0),
            target_(target)
        {}

        template <typename T>
        bool compare(T value)
        {
            next_ = value;
            return sum_ + next_ >= target_;
        }

        void next() {
            sum_ += next_;
        }

        size_t& local_pos() {return idx_;}
        const size_t& local_pos() const {return idx_;}

        FindGEWalker& idx(size_t value) {
            idx_ = value;
            return *this;
        }
    };

    struct FindGTWalker: WalkerBase {
    protected:
        IndexValue target_;
        IndexValue next_{};
        using WalkerBase::idx_;
        using WalkerBase::sum_;
    public:
        using WalkerBase::idx;

        FindGTWalker(IndexValue target):
            WalkerBase(0, 0),
            target_(target)
        {}

        template <typename T>
        bool compare(T value)
        {
            next_ = value;
            return sum_ + next_ > target_;
        }

        void next() {
            sum_ += next_;
        }

        size_t& local_pos() {return idx_;}
        const size_t& local_pos() const {return idx_;}

        FindGTWalker& idx(size_t value) {
            idx_ = value;
            return *this;
        }
    };



    auto find_ge(size_t block, IndexValue value) const
    {
        return find(block, FindGEWalker(value));
    }

    auto find_gt(size_t block, IndexValue value) const
    {
        return find(block, FindGTWalker(value));
    }

    auto find_ge_fw(size_t block, size_t start, IndexValue value) const
    {
        return walk_fw(block, start, FindGEWalker(value));
    }

    auto find_gt_fw(size_t block, size_t start, IndexValue value) const
    {
        return walk_fw(block, start, FindGTWalker(value));
    }


    auto find_ge_bw(size_t block, size_t start, IndexValue value) const
    {
        return walk_bw(block, start, FindGEWalker(value));
    }

    auto find_gt_bw(size_t block, size_t start, IndexValue value) const
    {
        return walk_bw(block, start, FindGTWalker(value));
    }


    template <typename Walker>
    auto find(size_t block, Walker&& walker) const
    {
        auto metadata = this->metadata();
        auto values = this->values(block);

        size_t size = metadata->size();

        if (this->element_size(block * SegmentsPerBlock + 1) == 0)
        {
            for (size_t c = 0; c < size; c++)
            {
                if (walker.compare(values[c]))
                {
                    return walker.idx(c);
                }
                else {
                    walker.next();
                }
            }

            return walker.idx(size);
        }
        else {
            TreeLayout data;

            this->compute_tree_layout(metadata, data);

            data.indexes = this->index(block);

            size_t idx = this->find_index(data, walker);

            if (idx >= 0)
            {
                idx <<= ValuesPerBranchLog2;

                for (size_t c = idx; c < size; c++)
                {
                    if (walker.compare(values[c]))
                    {
                        return walker.idx(c);
                    }
                    else {
                        walker.next();
                    }
                }

                return walker.idx(size);
            }
            else {
                return walker.idx(size);
            }
        }
    }



    template <typename Walker>
    auto walk_fw(size_t block, size_t start, Walker&& walker) const
    {
        auto metadata = this->metadata();
        auto values = this->values(block);

        size_t size = metadata->size();

        if (start >= size - ValuesPerBranch * 2)
        {
            for (size_t c = start; c < size; c++)
            {
                if (walker.compare(values[c]))
                {
                    return walker.idx(c);
                }
                else {
                    walker.next();
                }
            }

            return walker.idx(size);
        }
        else {
            size_t window_end = (start | ValuesPerBranchMask) + 1;

            for (size_t c = start; c < window_end; c++)
            {
                if (walker.compare(values[c]))
                {
                    return walker.idx(c);
                }
                else {
                    walker.next();
                }
            }

            TreeLayout data;

            this->compute_tree_layout(metadata, data);

            data.indexes = this->index(block);

            size_t idx = this->walk_index_fw(
                    data,
                    window_end >> ValuesPerBranchLog2,
                    data.levels_max,
                    std::forward<Walker>(walker)
            );

            if (idx >= 0)
            {
                idx <<= ValuesPerBranchLog2;

                for (size_t c = idx; c < size; c++)
                {
                    if (walker.compare(values[c]))
                    {
                        return walker.idx(c);
                    }
                    else {
                        walker.next();
                    }
                }

                return walker.idx(size);
            }
            else {
                return walker.idx(size);
            }
        }
    }



    template <typename Walker>
    auto walk_bw(size_t block, size_t start, Walker&& walker) const
    {
        auto metadata = this->metadata();
        auto values = this->values(block);

        if (start < ValuesPerBranch * 2)
        {
            for (size_t cc = start + 1; cc > 0; cc--)
            {
                size_t c = cc - 1;

                if (walker.compare(values[c]))
                {
                    return walker.idx(c);
                }
                else {
                    walker.next();
                }
            }

            return walker.idx(start + 1);
        }
        else {
            size_t window_end = (start & ~ValuesPerBranchMask) - 1;

            for (size_t c = start; c > window_end; c--)
            {
                if (walker.compare(values[c]))
                {
                    return walker.idx(c);
                }
                else {
                    walker.next();
                }
            }

            TreeLayout data;

            this->compute_tree_layout(metadata, data);

            data.indexes = this->index(block);

            size_t idx_start = window_end >> ValuesPerBranchLog2;
            size_t idx = this->walk_index_bw(
                    data,
                    idx_start,
                    data.levels_max,
                    std::forward<Walker>(walker)
            );

            if (idx <= idx_start)
            {
                size_t window_start = ((idx + 1) << ValuesPerBranchLog2) - 1;

                for (size_t cc = window_start + 1; cc > 0; cc--)
                {
                    size_t c = cc - 1;

                    if (walker.compare(values[c]))
                    {
                        return walker.idx(c);
                    }
                    else {
                        walker.next();
                    }
                }

                return walker.idx(start + 1);
            }
            else {
                return walker.idx(start + 1);
            }
        }
    }



    IndexValue sum(size_t block, size_t start, size_t end) const
    {
        TreeLayout layout;
        this->compute_tree_layout(metadata(), layout);

        return sum(layout, block, start, end);
    }

    IndexValue sum(size_t block, size_t end) const
    {
        TreeLayout layout;
        this->compute_tree_layout(metadata(), layout);

        return sum(layout, block, 0, end);
    }

    IndexValue sum(size_t block) const
    {
        return sum_for(metadata(), block);
    }

    IndexValue sum_for(const Metadata* meta, size_t block) const
    {
        if (this->element_size(block * SegmentsPerBlock + 1) == 0)
        {
            return sum(block, 0, meta->size());
        }
        else {
            auto index = this->index(block);
            return index[0];
        }
    }

    IndexValue sum(TreeLayout& layout, size_t block, size_t start, size_t end) const
    {
        IndexValue s = 0;

        size_t window_end   = (start | ValuesPerBranchMask) + 1;
        size_t window_start = end & ~ValuesPerBranchMask;

        auto values = this->values(block);

        if (end <= window_end || window_start - window_end <= ValuesPerBranch || layout.levels_max == -1)
        {
            for (size_t c = start; c < end; c++)
            {
                s += values[c];
            }
        }
        else {
            for (size_t c = start; c < window_end; c++)
            {
                s += values[c];
            }

            layout.indexes = this->index(block);

            this->sum_index(
                    layout,
                    s,
                    window_end >> ValuesPerBranchLog2,
                    window_start >> ValuesPerBranchLog2,
                    layout.levels_max
            );

            for (size_t c = window_start; c < end; c++)
            {
                s += values[c];
            }
        }

        return s;
    }

    void reindex(size_t blocks)
    {
        Metadata* meta = this->metadata();

        TreeLayout layout;
        size_t levels = this->compute_tree_layout(meta, layout);

        meta->set_index_size(layout.index_size);

        for (size_t block = 0; block < blocks; block++)
        {
            if (levels > 0)
            {
                this->reindex_block(meta, block, layout);
            }
        }
    }

    size_t size() const {
        return metadata()->size();
    }


    size_t index_size() const {
        return metadata()->index_size();
    }

    size_t max_size() const {
        return metadata()->max_size();
    }

    void dump_index(size_t blocks, std::ostream& out = std::cout) const
    {
        auto meta = this->metadata();

        out << "size_         = " << meta->size() << std::endl;
        out << "index_size_   = " << meta->index_size() << std::endl;

        out << std::endl;

        TreeLayout layout;

        size_t levels = this->compute_tree_layout(meta->max_size(), layout);

        if (levels > 0)
        {
            out << "TreeLayout: " << std::endl;

            out << "Level sizes: ";
            for (size_t c = 0; c <= layout.levels_max; c++) {
                out<<layout.level_sizes[c]<<" ";
            }
            out << std::endl;

            out<<"Level starts: ";
            for (size_t c = 0; c <= layout.levels_max; c++) {
                out<<layout.level_starts[c]<<" ";
            }
            out << std::endl;
        }

        for (size_t block = 0; block < blocks; block++)
        {
            out << "++++++++++++++++++ Block: " << block << " ++++++++++++++++++" << std::endl;

            if (levels > 0)
            {
                auto indexes = this->index(block);

                out << "Index:" << std::endl;
                for (size_t c = 0; c < meta->index_size(); c++)
                {
                    out << c << ": " << indexes[c] << std::endl;
                }
            }
        }

    }


    void dump(size_t blocks, std::ostream& out = std::cout, bool dump_index = true) const
    {
        auto meta = this->metadata();

        out << "size_         = " << meta->size() << std::endl;
        out << "index_size_   = " << meta->index_size() << std::endl;

        out << std::endl;

        TreeLayout layout;

        size_t levels = this->compute_tree_layout(meta->max_size(), layout);

        if (dump_index)
        {
            if (levels > 0)
            {
                out << "TreeLayout: " << std::endl;

                out << "Level sizes: ";
                for (size_t c = 0; c <= layout.levels_max; c++) {
                    out<<layout.level_sizes[c]<<" ";
                }
                out << std::endl;

                out << "Level starts: ";
                for (size_t c = 0; c <= layout.levels_max; c++) {
                    out<<layout.level_starts[c]<<" ";
                }
                out << std::endl;
            }
        }

        for (size_t block = 0; block < blocks; block++)
        {
            out << "++++++++++++++++++ Block: " << block << " ++++++++++++++++++" << std::endl;

            if (dump_index && levels > 0)
            {
                auto indexes = this->index(block);

                out << "Index:" << std::endl;
                for (size_t c = 0; c < meta->index_size(); c++)
                {
                    out << c << ": " << indexes[c] << std::endl;
                }
            }

            out << std::endl;

            out << "Values: " << std::endl;

            auto values = this->values(block);

            for (size_t c = 0; c < meta->size(); c++)
            {
                out << c << ": " << values[c] << std::endl;
            }
        }
    }



    auto findGTForward(size_t block, size_t start, IndexValue val) const
    {
        return this->find_gt_fw(block, start, val);
    }

    auto findGTForward(size_t block, IndexValue val) const
    {
        return this->find_gt(block, val);
    }

    auto findGTBackward(size_t block, size_t start, IndexValue val) const
    {
        return this->find_gt_bw(block, start, val);
    }

    auto findGTBackward(size_t block, IndexValue val) const
    {
        return this->find_gt_bw(block, this->size() - 1, val);
    }


    auto findGEForward(size_t block, size_t start, IndexValue val) const
    {
        return this->find_ge_fw(block, start, val);
    }

    auto findGEForward(size_t block, IndexValue val) const
    {
        return this->find_ge(block, val);
    }

    auto findGEBackward(size_t block, size_t start, IndexValue val) const
    {
        return this->find_ge_bw(block, start, val);
    }

    auto findGEBackward(size_t block, IndexValue val) const
    {
        return this->find_ge_bw(block, this->size() - 1, val);
    }

    class FindResult {
        IndexValue prefix_;
        size_t idx_;
    public:
        template <typename Fn>
        FindResult(Fn&& fn): prefix_(fn.prefix()), idx_(fn.local_pos()) {}

        IndexValue prefix() {return prefix_;}
        size_t local_pos() const {return idx_;}
    };

    auto findForward(SearchType search_type, size_t block, size_t start, IndexValue val) const
    {
        if (search_type == SearchType::GT)
        {
            return FindResult(findGTForward(block, start, val));
        }
        else {
            return FindResult(findGEForward(block, start, val));
        }
    }

    auto findBackward(SearchType search_type, size_t block, size_t start, IndexValue val) const
    {
        if (search_type == SearchType::GT)
        {
            return FindResult(findGTBackward(block, start, val));
        }
        else {
            return FindResult(findGEBackward(block, start, val));
        }
    }

    auto findForward(SearchType search_type, size_t block, IndexValue val) const
    {
        if (search_type == SearchType::GT)
        {
            return FindResult(findGTForward(block, val));
        }
        else {
            return FindResult(findGEForward(block, val));
        }
    }

    auto findBackward(SearchType search_type, size_t block, IndexValue val) const
    {
        if (search_type == SearchType::GT)
        {
            return FindResult(findGTBackward(block, val));
        }
        else {
            return FindResult(findGEBackward(block, val));
        }
    }

    size_t findNZ(size_t block, size_t start, size_t end) const
    {
    	auto values = this->values(block);

        for (size_t c = start; c < end; c++)
    	{
    		if (values[c] != 0) {
    			return c;
    		}
    	}

    	return end;
    }



    size_t findNZLT(size_t block, size_t start) const
    {
    	auto size = this->size();

    	MEMORIA_ASSERT(start, <, size);

        size_t min_idx = size;

        for (size_t b = 0; b < block; b++)
    	{
            size_t idx = findNZ(b, start, size);
    		if (idx < min_idx)
    		{
    			min_idx = idx;
    		}
    	}

    	return min_idx;
    }



    template <typename ConsumerFn>
    size_t scan(size_t block, size_t start, size_t end, ConsumerFn&& fn) const
    {
        auto values = this->values(block);

        size_t c;
        for (c = start; c < end; c++)
        {
            fn(c, values[c]);
        }

        return c;
    }

    template <typename T>
    void read(size_t block, size_t start, size_t end, T* values) const
    {
        MEMORIA_ASSERT(start, >=, 0);
        MEMORIA_ASSERT(start, <=, end);
        MEMORIA_ASSERT(end, <=, size());

        scan(block, start, end, [&](size_t c, auto value){
            values[c - start] = value;
        });
    }


protected:
    void reindex_block(const Metadata* meta, size_t block, TreeLayout& layout)
    {
        auto values = this->values(block);
        auto indexes = this->index(block);
        layout.indexes = indexes;

        size_t levels = layout.levels_max + 1;

        size_t level_start = layout.level_starts[levels - 1];
        size_t level_size = layout.level_sizes[levels - 1];

        size_t size = meta->size();

        for (int i = 0; i < level_size; i++)
        {
            IndexValue sum{};

            size_t start = i << ValuesPerBranchLog2;
            size_t window_end = (i + 1) << ValuesPerBranchLog2;

            size_t end = window_end <= size ? window_end : size;

            for (size_t c = start; c < end; c++) {
                sum += values[c];
            }

            indexes[level_start + i] = sum;
        }

        for (size_t level = levels - 1; level > 0; level--)
        {
            size_t previous_level_start = layout.level_starts[level - 1];
            size_t previous_level_size  = layout.level_sizes[level - 1];

            size_t current_level_start  = layout.level_starts[level];

            size_t current_level_size = layout.level_sizes[level];

            for (int i = 0; i < previous_level_size; i++)
            {
                IndexValue sum{};

                size_t start       = (i << BranchingFactorLog2) + current_level_start;
                size_t window_end  = ((i + 1) << BranchingFactorLog2);

                size_t end = (window_end <= current_level_size ? window_end : current_level_size) + current_level_start;

                for (size_t c = start; c < end; c++) {
                    sum += indexes[c];
                }

                indexes[previous_level_start + i] = sum;
            }
        }
    }
};

}
