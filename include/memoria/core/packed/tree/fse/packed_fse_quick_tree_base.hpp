
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

template <typename IndexValueT, typename ValueT, int32_t kBranchingFactor, int32_t kValuesPerBranch> class PkdFQTreeBase;

class PkdFQTreeMetadata {
    int32_t size_;
    int32_t index_size_;
    int32_t max_size_;
public:
    PkdFQTreeMetadata() = default;

    int32_t& size() {return size_;}
    const int32_t& size() const {return size_;}

    int32_t& index_size() {return index_size_;}
    const int32_t& index_size() const {return index_size_;}

    int32_t& max_size() {return max_size_;}
    const int32_t& max_size() const {return max_size_;}

    int32_t capacity() const {
        return max_size_ - size_;
    }

    template <typename, int32_t, int32_t, int32_t, typename> friend class PkdFQTreeBaseBase;
    template <typename, typename, int32_t, int32_t> friend class PkdFQTreeBase;
};



template <typename IndexValueT, typename ValueT, int32_t kBranchingFactor, int32_t kValuesPerBranch>
class PkdFQTreeBase: public PkdFQTreeBaseBase<IndexValueT, kBranchingFactor, kValuesPerBranch, 2, PkdFQTreeMetadata> {

    using Base      = PkdFQTreeBaseBase<IndexValueT, kBranchingFactor, kValuesPerBranch, 2, PkdFQTreeMetadata>;
    using MyType    = PkdFQTreeBase<IndexValueT, ValueT, kBranchingFactor, kValuesPerBranch>;

public:
    static constexpr uint32_t VERSION = 1;

    using IndexValue    = IndexValueT;
    using Value         = ValueT;

    using Metadata      = typename Base::Metadata;
    using TreeLayout    = typename Base::template IndexedTreeLayout<IndexValue>;


    static const int32_t BranchingFactor        = kBranchingFactor;
    static const int32_t ValuesPerBranch        = kValuesPerBranch;

    static const bool FixedSizeElement      = true;

    static constexpr int32_t ValuesPerBranchMask    = ValuesPerBranch - 1;
    static constexpr int32_t BranchingFactorMask    = BranchingFactor - 1;

    static constexpr int32_t ValuesPerBranchLog2    = Log2(ValuesPerBranch) - 1;
    static constexpr int32_t BranchingFactorLog2    = Log2(BranchingFactor) - 1;

    static constexpr int32_t SegmentsPerBlock = 2;

    static constexpr int32_t METADATA = 0;


    struct InitFn {
        int32_t blocks_;

        InitFn(int32_t blocks): blocks_(blocks) {}

        Int32Result block_size(int32_t items_number) const noexcept {
            return wrap_throwing([&](){
                return MyType::block_size(blocks_, items_number);
            });
        }

        int32_t max_elements(int32_t block_size)
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


    VoidResult init_tl(int32_t data_block_size, int32_t blocks) noexcept
    {
        MEMORIA_TRY_VOID(Base::init(data_block_size, blocks * SegmentsPerBlock + 1));

        MEMORIA_TRY(meta, this->template allocate<Metadata>(METADATA));

        int32_t max_size        = 0;

        meta->size()        = 0;
        meta->max_size()    = max_size;
        meta->index_size()  = MyType::index_size(max_size);

        for (int32_t block = 0; block < blocks; block++)
        {
            MEMORIA_TRY_VOID(this->template allocateArrayBySize<IndexValue>(block * SegmentsPerBlock + 1, meta->index_size()));
            MEMORIA_TRY_VOID(this->template allocateArrayBySize<Value>(block * SegmentsPerBlock + 2, max_size));
        }

        return VoidResult::of();
    }



    static int32_t block_size(int32_t blocks, int32_t capacity) noexcept
    {
        int32_t metadata_length = PackedAllocatable::roundUpBytesToAlignmentBlocks(sizeof(Metadata));

        int32_t index_size      = MyType::index_size(capacity);
        int32_t index_length    = PackedAllocatable::roundUpBytesToAlignmentBlocks(index_size * sizeof(IndexValue));

        int32_t values_length   = PackedAllocatable::roundUpBytesToAlignmentBlocks(capacity * sizeof(Value));

        return Base::block_size(metadata_length + (index_length + values_length) * blocks, blocks * SegmentsPerBlock + 1);
    }

    int32_t block_size() const
    {
        return Base::block_size();
    }

    static int32_t index_size(int32_t capacity)
    {
        TreeLayout layout;
        Base::compute_tree_layout(capacity, layout);
        return layout.index_size;
    }

    static Int32Result tree_size(int32_t blocks, int32_t block_size) noexcept
    {
        if (block_size >= (int32_t)sizeof(Value)) {
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

    IndexValue* index(int32_t block) {
        return Base::template index<IndexValue, 0>(block);
    }

    const IndexValue* index(int32_t block) const {
        return Base::template index<IndexValue, 0>(block);
    }



    Value* values(int32_t block) {
        return this->template get<Value>(block * SegmentsPerBlock + 2);
    }
    const Value* values(int32_t block) const {
        return this->template get<Value>(block * SegmentsPerBlock + 2);
    }

    Value& value(int32_t block, int32_t idx) {
        return values(block)[idx];
    }

    const Value& value(int32_t block, int32_t idx) const {
        return values(block)[idx];
    }

    class WalkerBase {
    protected:
        int32_t idx_;
        IndexValue sum_;
    public:
        WalkerBase(int32_t idx, IndexValue sum): idx_(idx), sum_(sum) {}

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

        int32_t& local_pos() {return idx_;}
        const int32_t& local_pos() const {return idx_;}

        FindGEWalker& idx(int32_t value) {
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

        int32_t& local_pos() {return idx_;}
        const int32_t& local_pos() const {return idx_;}

        FindGTWalker& idx(int32_t value) {
            idx_ = value;
            return *this;
        }
    };



    auto find_ge(int32_t block, IndexValue value) const
    {
        return find(block, FindGEWalker(value));
    }

    auto find_gt(int32_t block, IndexValue value) const
    {
        return find(block, FindGTWalker(value));
    }

    auto find_ge_fw(int32_t block, int32_t start, IndexValue value) const
    {
        return walk_fw(block, start, FindGEWalker(value));
    }

    auto find_gt_fw(int32_t block, int32_t start, IndexValue value) const
    {
        return walk_fw(block, start, FindGTWalker(value));
    }


    auto find_ge_bw(int32_t block, int32_t start, IndexValue value) const
    {
        return walk_bw(block, start, FindGEWalker(value));
    }

    auto find_gt_bw(int32_t block, int32_t start, IndexValue value) const
    {
        return walk_bw(block, start, FindGTWalker(value));
    }


    template <typename Walker>
    auto find(int32_t block, Walker&& walker) const
    {
        auto metadata = this->metadata();
        auto values = this->values(block);

        int32_t size = metadata->size();

        if (this->element_size(block * SegmentsPerBlock + 1) == 0)
        {
            for (int32_t c = 0; c < size; c++)
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

            int32_t idx = this->find_index(data, walker);

            if (idx >= 0)
            {
                idx <<= ValuesPerBranchLog2;

                for (int32_t c = idx; c < size; c++)
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
    auto walk_fw(int32_t block, int32_t start, Walker&& walker) const
    {
        auto metadata = this->metadata();
        auto values = this->values(block);

        int32_t size = metadata->size();

        if (start >= size - ValuesPerBranch * 2)
        {
            for (int32_t c = start; c < size; c++)
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
            int32_t window_end = (start | ValuesPerBranchMask) + 1;

            for (int32_t c = start; c < window_end; c++)
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

            int32_t idx = this->walk_index_fw(
                    data,
                    window_end >> ValuesPerBranchLog2,
                    data.levels_max,
                    std::forward<Walker>(walker)
            );

            if (idx >= 0)
            {
                idx <<= ValuesPerBranchLog2;

                for (int32_t c = idx; c < size; c++)
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
    auto walk_bw(int32_t block, int32_t start, Walker&& walker) const
    {
        auto metadata = this->metadata();
        auto values = this->values(block);

        if (start < ValuesPerBranch * 2)
        {
            for (int32_t c = start; c >= 0; c--)
            {
                if (walker.compare(values[c]))
                {
                    return walker.idx(c);
                }
                else {
                    walker.next();
                }
            }

            return walker.idx(-1);
        }
        else {
            int32_t window_end = (start & ~ValuesPerBranchMask) - 1;

            for (int32_t c = start; c > window_end; c--)
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

            int32_t idx = this->walk_index_bw(
                    data,
                    window_end >> ValuesPerBranchLog2,
                    data.levels_max,
                    std::forward<Walker>(walker)
            );

            if (idx >= 0)
            {
                int32_t window_start = ((idx + 1) << ValuesPerBranchLog2) - 1;

                for (int32_t c = window_start; c >= 0; c--)
                {
                    if (walker.compare(values[c]))
                    {
                        return walker.idx(c);
                    }
                    else {
                        walker.next();
                    }
                }

                return walker.idx(-1);
            }
            else {
                return walker.idx(-1);
            }
        }
    }



    IndexValue sum(int32_t block, int32_t start, int32_t end) const
    {
        TreeLayout layout;
        this->compute_tree_layout(metadata(), layout);

        return sum(layout, block, start, end);
    }

    IndexValue sum(int32_t block, int32_t end) const
    {
        TreeLayout layout;
        this->compute_tree_layout(metadata(), layout);

        return sum(layout, block, 0, end);
    }

    IndexValue sum(int32_t block) const
    {
        return sum(metadata(), block);
    }

    IndexValue sum(const Metadata* meta, int32_t block) const
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

    IndexValue sum(TreeLayout& layout, int32_t block, int32_t start, int32_t end) const
    {
        IndexValue s = 0;

        int32_t window_end   = (start | ValuesPerBranchMask) + 1;
        int32_t window_start = end & ~ValuesPerBranchMask;

        auto values = this->values(block);

        if (end <= window_end || window_start - window_end <= ValuesPerBranch || layout.levels_max == -1)
        {
            for (int32_t c = start; c < end; c++)
            {
                s += values[c];
            }
        }
        else {
            for (int32_t c = start; c < window_end; c++)
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

            for (int32_t c = window_start; c < end; c++)
            {
                s += values[c];
            }
        }

        return s;
    }

    void reindex(int32_t blocks)
    {
        Metadata* meta = this->metadata();

        TreeLayout layout;
        int32_t levels = this->compute_tree_layout(meta, layout);

        meta->index_size() = layout.index_size;

        for (int32_t block = 0; block < blocks; block++)
        {
            if (levels > 0)
            {
                this->reindex_block(meta, block, layout);
            }
        }
    }

    const int32_t& size() const {
        return metadata()->size();
    }

    int32_t& size() {
        return metadata()->size();
    }

    int32_t index_size() const {
        return metadata()->index_size();
    }

    int32_t max_size() const {
        return metadata()->max_size();
    }

    void dump_index(int32_t blocks, std::ostream& out = std::cout) const
    {
        auto meta = this->metadata();

        out << "size_         = " << meta->size() << std::endl;
        out << "index_size_   = " << meta->index_size() << std::endl;

        out << std::endl;

        TreeLayout layout;

        int32_t levels = this->compute_tree_layout(meta->max_size(), layout);

        if (levels > 0)
        {
            out << "TreeLayout: " << std::endl;

            out << "Level sizes: ";
            for (int32_t c = 0; c <= layout.levels_max; c++) {
                out<<layout.level_sizes[c]<<" ";
            }
            out << std::endl;

            out<<"Level starts: ";
            for (int32_t c = 0; c <= layout.levels_max; c++) {
                out<<layout.level_starts[c]<<" ";
            }
            out << std::endl;
        }

        for (int32_t block = 0; block < blocks; block++)
        {
            out << "++++++++++++++++++ Block: " << block << " ++++++++++++++++++" << std::endl;

            if (levels > 0)
            {
                auto indexes = this->index(block);

                out << "Index:" << std::endl;
                for (int32_t c = 0; c < meta->index_size(); c++)
                {
                    out << c << ": " << indexes[c] << std::endl;
                }
            }
        }

    }


    void dump(int32_t blocks, std::ostream& out = std::cout, bool dump_index = true) const
    {
        auto meta = this->metadata();

        out << "size_         = " << meta->size() << std::endl;
        out << "index_size_   = " << meta->index_size() << std::endl;

        out << std::endl;

        TreeLayout layout;

        int32_t levels = this->compute_tree_layout(meta->max_size(), layout);

        if (dump_index)
        {
            if (levels > 0)
            {
                out << "TreeLayout: " << std::endl;

                out << "Level sizes: ";
                for (int32_t c = 0; c <= layout.levels_max; c++) {
                    out<<layout.level_sizes[c]<<" ";
                }
                out << std::endl;

                out << "Level starts: ";
                for (int32_t c = 0; c <= layout.levels_max; c++) {
                    out<<layout.level_starts[c]<<" ";
                }
                out << std::endl;
            }
        }

        for (int32_t block = 0; block < blocks; block++)
        {
            out << "++++++++++++++++++ Block: " << block << " ++++++++++++++++++" << std::endl;

            if (dump_index && levels > 0)
            {
                auto indexes = this->index(block);

                out << "Index:" << std::endl;
                for (int32_t c = 0; c < meta->index_size(); c++)
                {
                    out << c << ": " << indexes[c] << std::endl;
                }
            }

            out << std::endl;

            out << "Values: " << std::endl;

            auto values = this->values(block);

            for (int32_t c = 0; c < meta->size(); c++)
            {
                out << c << ": " << values[c] << std::endl;
            }
        }
    }



    auto findGTForward(int32_t block, int32_t start, IndexValue val) const
    {
        return this->find_gt_fw(block, start, val);
    }

    auto findGTForward(int32_t block, IndexValue val) const
    {
        return this->find_gt(block, val);
    }

    auto findGTBackward(int32_t block, int32_t start, IndexValue val) const
    {
        return this->find_gt_bw(block, start, val);
    }

    auto findGTBackward(int32_t block, IndexValue val) const
    {
        return this->find_gt_bw(block, this->size() - 1, val);
    }


    auto findGEForward(int32_t block, int32_t start, IndexValue val) const
    {
        return this->find_ge_fw(block, start, val);
    }

    auto findGEForward(int32_t block, IndexValue val) const
    {
        return this->find_ge(block, val);
    }

    auto findGEBackward(int32_t block, int32_t start, IndexValue val) const
    {
        return this->find_ge_bw(block, start, val);
    }

    auto findGEBackward(int32_t block, IndexValue val) const
    {
        return this->find_ge_bw(block, this->size() - 1, val);
    }

    class FindResult {
        IndexValue prefix_;
        int32_t idx_;
    public:
        template <typename Fn>
        FindResult(Fn&& fn): prefix_(fn.prefix()), idx_(fn.local_pos()) {}

        IndexValue prefix() {return prefix_;}
        int32_t local_pos() const {return idx_;}
    };

    auto findForward(SearchType search_type, int32_t block, int32_t start, IndexValue val) const
    {
        if (search_type == SearchType::GT)
        {
            return FindResult(findGTForward(block, start, val));
        }
        else {
            return FindResult(findGEForward(block, start, val));
        }
    }

    auto findBackward(SearchType search_type, int32_t block, int32_t start, IndexValue val) const
    {
        if (search_type == SearchType::GT)
        {
            return FindResult(findGTBackward(block, start, val));
        }
        else {
            return FindResult(findGEBackward(block, start, val));
        }
    }

    auto findForward(SearchType search_type, int32_t block, IndexValue val) const
    {
        if (search_type == SearchType::GT)
        {
            return FindResult(findGTForward(block, val));
        }
        else {
            return FindResult(findGEForward(block, val));
        }
    }

    auto findBackward(SearchType search_type, int32_t block, IndexValue val) const
    {
        if (search_type == SearchType::GT)
        {
            return FindResult(findGTBackward(block, val));
        }
        else {
            return FindResult(findGEBackward(block, val));
        }
    }

    int32_t findNZ(int32_t block, int32_t start, int32_t end) const
    {
    	auto values = this->values(block);

    	for (int32_t c = start; c < end; c++)
    	{
    		if (values[c] != 0) {
    			return c;
    		}
    	}

    	return end;
    }



    int32_t findNZLT(int32_t block, int32_t start) const
    {
    	auto size = this->size();

    	MEMORIA_ASSERT(start, <, size);

    	int32_t min_idx = size;

    	for (int32_t b = 0; b < block; b++)
    	{
    		int32_t idx = findNZ(b, start, size);
    		if (idx < min_idx)
    		{
    			min_idx = idx;
    		}
    	}

    	return min_idx;
    }



    template <typename ConsumerFn>
    int32_t scan(int32_t block, int32_t start, int32_t end, ConsumerFn&& fn) const
    {
        auto values = this->values(block);

        int32_t c;
        for (c = start; c < end; c++)
        {
            fn(c, values[c]);
        }

        return c;
    }

    template <typename T>
    void read(int32_t block, int32_t start, int32_t end, T* values) const
    {
        MEMORIA_ASSERT(start, >=, 0);
        MEMORIA_ASSERT(start, <=, end);
        MEMORIA_ASSERT(end, <=, size());

        scan(block, start, end, [&](int32_t c, auto value){
            values[c - start] = value;
        });
    }


protected:
    void reindex_block(const Metadata* meta, int32_t block, TreeLayout& layout)
    {
        auto values = this->values(block);
        auto indexes = this->index(block);
        layout.indexes = indexes;

        int32_t levels = layout.levels_max + 1;

        int32_t level_start = layout.level_starts[levels - 1];
        int32_t level_size = layout.level_sizes[levels - 1];

        int32_t size = meta->size();

        for (int i = 0; i < level_size; i++)
        {
            IndexValue sum{};

            int32_t start = i << ValuesPerBranchLog2;
            int32_t window_end = (i + 1) << ValuesPerBranchLog2;

            int32_t end = window_end <= size ? window_end : size;

            for (int32_t c = start; c < end; c++) {
                sum += values[c];
            }

            indexes[level_start + i] = sum;
        }

        for (int32_t level = levels - 1; level > 0; level--)
        {
            int32_t previous_level_start = layout.level_starts[level - 1];
            int32_t previous_level_size  = layout.level_sizes[level - 1];

            int32_t current_level_start  = layout.level_starts[level];

            int32_t current_level_size = layout.level_sizes[level];

            for (int i = 0; i < previous_level_size; i++)
            {
                IndexValue sum{};

                int32_t start       = (i << BranchingFactorLog2) + current_level_start;
                int32_t window_end  = ((i + 1) << BranchingFactorLog2);

                int32_t end = (window_end <= current_level_size ? window_end : current_level_size) + current_level_start;

                for (int32_t c = start; c < end; c++) {
                    sum += indexes[c];
                }

                indexes[previous_level_start + i] = sum;
            }
        }
    }
};

}
