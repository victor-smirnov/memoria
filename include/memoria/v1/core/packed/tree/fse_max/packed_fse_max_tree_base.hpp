
// Copyright 2016 Victor Smirnov
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

#include <memoria/v1/core/packed/tree/fse_max/packed_fse_max_tree_base_base.hpp>
#include <memoria/v1/core/tools/assert.hpp>

namespace memoria {
namespace v1 {

template <typename ValueT, int32_t kBranchingFactor, int32_t kValuesPerBranch> class PkdFMTreeBase;

class PkdFMTreeMetadata {
    int32_t size_;
    int32_t index_size_;
    int32_t max_size_;
public:
    PkdFMTreeMetadata() = default;

    int32_t& size() {return size_;}
    const int32_t& size() const {return size_;}

    int32_t& index_size() {return index_size_;}
    const int32_t& index_size() const {return index_size_;}

    int32_t& max_size() {return max_size_;}
    const int32_t& max_size() const {return max_size_;}

    int32_t capacity() const {
        return max_size_ - size_;
    }

    template <typename, int32_t, int32_t, int32_t, typename> friend class PkdFMTreeBaseBase;
    template <typename, int32_t, int32_t> friend class PkdFMTreeBase;
};



template <typename ValueT, int32_t kBranchingFactor, int32_t kValuesPerBranch>
class PkdFMTreeBase: public PkdFMTreeBaseBase<ValueT, kBranchingFactor, kValuesPerBranch, 2, PkdFMTreeMetadata> {

    using Base      = PkdFMTreeBaseBase<ValueT, kBranchingFactor, kValuesPerBranch, 2, PkdFMTreeMetadata>;
    using MyType    = PkdFMTreeBase<ValueT, kBranchingFactor, kValuesPerBranch>;

public:
    static constexpr uint32_t VERSION = 1;


    using Value         = ValueT;
    using IndexValue    = ValueT;

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

        int32_t block_size(int32_t items_number) const {
            return MyType::block_size(blocks_, items_number);
        }

        int32_t max_elements(int32_t block_size)
        {
            return block_size;
        }
    };

public:

    PkdFMTreeBase() = default;

    using FieldsList = MergeLists<
                typename Base::FieldsList,
                decltype(Metadata::max_size_),
                ConstValue<uint32_t, VERSION>,
                Value
    >;


    void init_tl(int32_t data_block_size, int32_t blocks)
    {
        Base::init(data_block_size, blocks * SegmentsPerBlock + 1);

        Metadata* meta = this->template allocate<Metadata>(METADATA);

        int32_t max_size        = 0;

        meta->size()        = 0;
        meta->max_size()    = max_size;
        meta->index_size()  = MyType::index_size(max_size);

        for (int32_t block = 0; block < blocks; block++)
        {
            this->template allocateArrayBySize<IndexValue>(block * SegmentsPerBlock + 1, meta->index_size());
            this->template allocateArrayBySize<Value>(block * SegmentsPerBlock + 2, max_size);
        }
    }



    static int32_t block_size(int32_t blocks, int32_t capacity)
    {
        int32_t metadata_length = Base::roundUpBytesToAlignmentBlocks(sizeof(Metadata));

        int32_t index_size      = MyType::index_size(capacity);
        int32_t index_length    = Base::roundUpBytesToAlignmentBlocks(index_size * sizeof(IndexValue));

        int32_t values_length   = Base::roundUpBytesToAlignmentBlocks(capacity * sizeof(Value));

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

    static int32_t tree_size(int32_t blocks, int32_t block_size)
    {
        return block_size >= (int32_t)sizeof(Value) ? FindTotalElementsNumber2(block_size, InitFn(blocks)) : 0;
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


    bool has_index() const {
        return true;
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


    struct FindGEWalker {
        IndexValue target_;
        int32_t idx_;
    public:
        FindGEWalker(const IndexValue& target): target_(target) {}

        template <typename T>
        bool compare(T&& value)
        {
            return value >= target_;
        }

        void next() {}

        int32_t& idx() {return idx_;}
        const int32_t& idx() const {return idx_;}

        FindGEWalker& idx(int32_t value) {
            idx_ = value;
            return *this;
        }
    };

    struct FindGTWalker {
        IndexValue target_;

        int32_t idx_;
    public:
        FindGTWalker(const IndexValue& target): target_(target) {}

        template <typename T>
        bool compare(T&& value)
        {
            return value > target_;
        }

        void next() {}

        int32_t& idx() {return idx_;}
        const int32_t& idx() const {return idx_;}

        FindGTWalker& idx(int32_t value) {
            idx_ = value;
            return *this;
        }
    };



    auto find_ge(int32_t block, const IndexValue& value) const
    {
        return find(block, FindGEWalker(value));
    }

    auto find_gt(int32_t block, const IndexValue& value) const
    {
        return find(block, FindGTWalker(value));
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

    void check(int32_t blocks) const
    {
        const Metadata* meta = this->metadata();

        TreeLayout layout;
        int32_t levels = this->compute_tree_layout(meta, layout);

        MEMORIA_V1_ASSERT(meta->index_size(), ==, layout.index_size);

        for (int32_t block = 0; block < blocks; block++)
        {
//          auto values = this->values(block);
//          for (int32_t c = 1; c < meta->size(); c++)
//          {
//              MEMORIA_V1_ASSERT(values[c - 1], <=, values[c]);
//          }

            if (levels > 0)
            {
                this->check_block(meta, block, layout);
            }
            else {
                MEMORIA_V1_ASSERT(Base::element_size(block * SegmentsPerBlock + 1), ==, 0);
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

        out << "size_         = "<<meta->size() << std::endl;
        out << "index_size_   = "<<meta->index_size() << std::endl;

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

        out<<std::endl;

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




    auto findGTForward(int32_t block, const IndexValue& val) const
    {
        return this->find_gt(block, val);
    }

    auto findGEForward(int32_t block, const IndexValue& val) const
    {
        return this->find_ge(block, val);
    }


    class FindResult {
        int32_t idx_;
    public:
        template <typename Fn>
        FindResult(Fn&& fn): idx_(fn.idx()) {}

        int32_t idx() const {return idx_;}

        void set_idx(int32_t idx)
        {
            this->idx_ = idx;
        }
    };


    auto findForward(SearchType search_type, int32_t block, const IndexValue& val) const
    {
        if (search_type == SearchType::GT)
        {
            return FindResult(findGTForward(block, val));
        }
        else {
            return FindResult(findGEForward(block, val));
        }
    }

    auto findForward(SearchType search_type, int32_t block, const OptionalT<IndexValue>& val) const
    {
        if (search_type == SearchType::GT)
        {
            return FindResult(findGTForward(block, val.value()));
        }
        else {
            return FindResult(findGEForward(block, val.value()));
        }
    }


    auto findBackward(SearchType search_type, int32_t block, const IndexValue& val) const
    {
        if (search_type == SearchType::GT)
        {
            return FindResult(findGTBackward(block, val));
        }
        else {
            return FindResult(findGEBackward(block, val));
        }
    }

    auto findBackward(SearchType search_type, int32_t block, const OptionalT<IndexValue>& val) const
    {
        if (search_type == SearchType::GT)
        {
            return FindResult(findGTBackward(block, val.value()));
        }
        else {
            return FindResult(findGEBackward(block, val.value()));
        }
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
        MEMORIA_V1_ASSERT(start, >=, 0);
        MEMORIA_V1_ASSERT(start, <=, end);
        MEMORIA_V1_ASSERT(end, <=, size());

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
            int32_t window_end = (i + 1) << ValuesPerBranchLog2;

            int32_t end = window_end <= size ? window_end : size;

            indexes[level_start + i] = values[end - 1];
        }

        for (int32_t level = levels - 1; level > 0; level--)
        {
            int32_t previous_level_start = layout.level_starts[level - 1];
            int32_t previous_level_size  = layout.level_sizes[level - 1];

            int32_t current_level_start  = layout.level_starts[level];

            int32_t current_level_size = layout.level_sizes[level];

            for (int i = 0; i < previous_level_size; i++)
            {
                int32_t window_end  = ((i + 1) << BranchingFactorLog2);

                int32_t end = (window_end <= current_level_size ? window_end : current_level_size) + current_level_start;

                indexes[previous_level_start + i] = indexes[end - 1];
            }
        }
    }


    void check_block(const Metadata* meta, int32_t block, TreeLayout& layout) const
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
            int32_t window_end = (i + 1) << ValuesPerBranchLog2;

            int32_t end = window_end <= size ? window_end : size;

            MEMORIA_V1_ASSERT(indexes[level_start + i], ==, values[end - 1]);
        }

        for (int32_t level = levels - 1; level > 0; level--)
        {
            int32_t previous_level_start = layout.level_starts[level - 1];
            int32_t previous_level_size  = layout.level_sizes[level - 1];

            int32_t current_level_start  = layout.level_starts[level];

            int32_t current_level_size = layout.level_sizes[level];

            for (int i = 0; i < previous_level_size; i++)
            {
                int32_t window_end  = ((i + 1) << BranchingFactorLog2);

                int32_t end = (window_end <= current_level_size ? window_end : current_level_size) + current_level_start;

                MEMORIA_V1_ASSERT(indexes[previous_level_start + i], ==, indexes[end - 1]);
            }
        }
    }
};


}}
