
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

#include <memoria/v1/prototypes/bt/bt_walkers.hpp>

#include <memoria/v1/core/packed/sseq/packed_fse_searchable_seq.hpp>

namespace memoria {
namespace v1 {
namespace louds     {




template <typename Types>
class SelectForwardWalker: public bt::FindForwardWalkerBase<Types, SelectForwardWalker<Types>> {
    typedef bt::FindForwardWalkerBase<Types, SelectForwardWalker<Types>>        Base;
    typedef typename Base::Key                                                  Key;

    int64_t pos_     = 0;
    int64_t rank1_   = 0;

public:
    typedef typename Base::ResultType                                           ResultType;
    typedef typename Base::Iterator                                             Iterator;


    SelectForwardWalker(int32_t stream, int32_t index, Key target): Base(stream, index + 1, target)
    {
        Base::search_type_ = SearchType::GE;
    }

    template <int32_t Idx, typename Tree>
    ResultType stream(const Tree* tree, int32_t start)
    {
        return Base::template stream<Idx>(tree, start);
    }

    template <int32_t StreamIdx, typename StreamType, typename Result>
    void postProcessStream(const StreamType* stream, int32_t start, const Result& result)
    {
        int32_t size    = stream->size();

        if (result.local_pos() < size)
        {
            pos_ += stream->sum(0, start, result.local_pos());
        }
        else {
            pos_ += stream->sum(0, start, size);
        }
    }

    template <int32_t Idx, typename StreamTypes>
    ResultType stream(const PkdFSSeq<StreamTypes>* seq, int32_t start)
    {
        MEMORIA_V1_ASSERT_TRUE(seq != nullptr);

        auto& sum       = Base::sum_;
        auto symbol     = Base::index_ - 1;

        int64_t target   = Base::target_ - sum;
        auto result     = seq->selectFw(start, symbol, target);

        if (result.is_found())
        {
            pos_ += result.local_pos() - start;
            sum  += target;

            return result.local_pos();
        }
        else {
            int32_t size = seq->size();

            sum  += result.rank();
            pos_ += (size - start);

            return size;
        }
    }

    void prepare(Iterator& iter)
    {
        MEMORIA_V1_ASSERT_TRUE(!iter.isBof());
    }

    int64_t finish(Iterator& iter, int32_t idx)
    {
        iter.local_pos() = idx;

        int32_t symbol = this->index_ - 1;
        auto sum   = this->sum_;

        if (!iter.isEof())
        {
            sum -= iter.symbol() == symbol;
        }

        int64_t rank1 =  symbol ? sum : pos_ - sum;

        iter.iter_cache().add(pos_, rank1);

        return pos_;
    }
};



template <typename Types>
class SelectBackwardWalker: public bt::FindBackwardWalkerBase<Types, SelectBackwardWalker<Types>> {
    typedef bt::FindBackwardWalkerBase<Types, SelectBackwardWalker<Types>>      Base;
    typedef typename Base::Key                                                  Key;

    int64_t pos_     = 0;

public:
    typedef typename Base::ResultType                                           ResultType;
    typedef typename Base::Iterator                                             Iterator;

    SelectBackwardWalker(int32_t stream, int32_t index, Key target): Base(stream, index + 1, target)
    {
        Base::search_type_ = SearchType::GT;
    }

    template <int32_t Idx, typename Tree>
    ResultType stream(const Tree* tree, int32_t start)
    {
        return Base::template stream<Idx>(tree, start);
    }

    template <int32_t StreamIdx, typename StreamType, typename Result>
    void postProcessStream(const StreamType* stream, int32_t start, const Result& result)
    {
        if (result.local_pos() >= 0)
        {
            pos_ += stream->sum(0, result.local_pos() + 1, start + 1);
        }
        else {
            pos_ += stream->sum(0, 0, start + 1);
        }
    }


    template <int32_t Idx, typename TreeTypes>
    ResultType stream(const PkdFSSeq<TreeTypes>* seq, int32_t start)
    {
        MEMORIA_V1_ASSERT_TRUE(seq != nullptr);

        int64_t target   = Base::target_ - Base::sum_;

        auto& sum       = Base::sum_;
        auto symbol     = Base::index_ - 1;

        auto result     = seq->selectBw(start, symbol, target);

        if (result.is_found())
        {
            pos_ += start - result.local_pos();
            sum  += target;

            return result.local_pos();
        }
        else {
            pos_ += start;
            sum  += result.rank();

            return -1;
        }
    }

    void prepare(Iterator& iter)
    {
        MEMORIA_V1_ASSERT_TRUE(!iter.isBof());
    }

    int64_t finish(Iterator& iter, int32_t idx)
    {
        iter.local_pos() = idx;

        int32_t symbol = this->index_ - 1;
        auto sum   = this->sum_;

        if (idx >= 0)
        {
            int64_t rank1 =  symbol ? sum : pos_ - sum;

            iter.iter_cache().sub(pos_, rank1);
        }
        else {
            iter.iter_cache().setup(-1, 0);
        }

        return pos_;
    }
};

}
}}