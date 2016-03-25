
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

    BigInt pos_     = 0;
    BigInt rank1_   = 0;

public:
    typedef typename Base::ResultType                                           ResultType;
    typedef typename Base::Iterator                                             Iterator;


    SelectForwardWalker(Int stream, Int index, Key target): Base(stream, index + 1, target)
    {
        Base::search_type_ = SearchType::GE;
    }

    template <Int Idx, typename Tree>
    ResultType stream(const Tree* tree, Int start)
    {
        return Base::template stream<Idx>(tree, start);
    }

    template <Int StreamIdx, typename StreamType, typename Result>
    void postProcessStream(const StreamType* stream, Int start, const Result& result)
    {
        Int size    = stream->size();

        if (result.idx() < size)
        {
            pos_ += stream->sum(0, start, result.idx());
        }
        else {
            pos_ += stream->sum(0, start, size);
        }
    }

    template <Int Idx, typename StreamTypes>
    ResultType stream(const PkdFSSeq<StreamTypes>* seq, Int start)
    {
        MEMORIA_V1_ASSERT_TRUE(seq != nullptr);

        auto& sum       = Base::sum_;
        auto symbol     = Base::index_ - 1;

        BigInt target   = Base::target_ - sum;
        auto result     = seq->selectFw(start, symbol, target);

        if (result.is_found())
        {
            pos_ += result.idx() - start;
            sum  += target;

            return result.idx();
        }
        else {
            Int size = seq->size();

            sum  += result.rank();
            pos_ += (size - start);

            return size;
        }
    }

    void prepare(Iterator& iter)
    {
        MEMORIA_V1_ASSERT_TRUE(!iter.isBof());
    }

    BigInt finish(Iterator& iter, Int idx)
    {
        iter.idx() = idx;

        Int symbol = this->index_ - 1;
        auto sum   = this->sum_;

        if (!iter.isEof())
        {
            sum -= iter.symbol() == symbol;
        }

        BigInt rank1 =  symbol ? sum : pos_ - sum;

        iter.cache().add(pos_, rank1);

        return pos_;
    }
};



template <typename Types>
class SelectBackwardWalker: public bt::FindBackwardWalkerBase<Types, SelectBackwardWalker<Types>> {
    typedef bt::FindBackwardWalkerBase<Types, SelectBackwardWalker<Types>>      Base;
    typedef typename Base::Key                                                  Key;

    BigInt pos_     = 0;

public:
    typedef typename Base::ResultType                                           ResultType;
    typedef typename Base::Iterator                                             Iterator;

    SelectBackwardWalker(Int stream, Int index, Key target): Base(stream, index + 1, target)
    {
        Base::search_type_ = SearchType::GT;
    }

    template <Int Idx, typename Tree>
    ResultType stream(const Tree* tree, Int start)
    {
        return Base::template stream<Idx>(tree, start);
    }

    template <Int StreamIdx, typename StreamType, typename Result>
    void postProcessStream(const StreamType* stream, Int start, const Result& result)
    {
        if (result.idx() >= 0)
        {
            pos_ += stream->sum(0, result.idx() + 1, start + 1);
        }
        else {
            pos_ += stream->sum(0, 0, start + 1);
        }
    }


    template <Int Idx, typename TreeTypes>
    ResultType stream(const PkdFSSeq<TreeTypes>* seq, Int start)
    {
        MEMORIA_V1_ASSERT_TRUE(seq != nullptr);

        BigInt target   = Base::target_ - Base::sum_;

        auto& sum       = Base::sum_;
        auto symbol     = Base::index_ - 1;

        auto result     = seq->selectBw(start, symbol, target);

        if (result.is_found())
        {
            pos_ += start - result.idx();
            sum  += target;

            return result.idx();
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

    BigInt finish(Iterator& iter, Int idx)
    {
        iter.idx() = idx;

        Int symbol = this->index_ - 1;
        auto sum   = this->sum_;

        if (idx >= 0)
        {
            BigInt rank1 =  symbol ? sum : pos_ - sum;

            iter.cache().sub(pos_, rank1);
        }
        else {
            iter.cache().setup(-1, 0);
        }

        return pos_;
    }
};

}
}}