
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
class SkipForwardWalker: public bt::FindForwardWalkerBase<Types, SkipForwardWalker<Types>> {
    typedef bt::FindForwardWalkerBase<Types, SkipForwardWalker<Types>>          Base;
    typedef typename Base::Key                                                  Key;

    int64_t rank1_ = 0;

public:
    typedef typename Base::ResultType                                           ResultType;
    typedef typename Base::Iterator                                             Iterator;


    SkipForwardWalker(int32_t stream, int32_t index, Key target): Base(stream, index, target)
    {}

    template <int32_t Idx, typename Tree>
    ResultType stream(Tree* tree, int32_t start)
    {
        return Base::template stream<Idx>(tree, start);
    }

    template <int32_t StreamIdx, typename StreamType, typename Result>
    void postProcessStream(const StreamType* stream, int32_t start, const Result& result)
    {
        int32_t size = stream->size();

        if (result.local_pos() < size)
        {
            rank1_ += stream->sum(2, start, result.local_pos());
        }
        else {
            rank1_ += stream->sum(2, start, size);
        }
    }

    template <int32_t Idx, typename StreamTypes>
    ResultType stream(const PkdFSSeq<StreamTypes>* seq, int32_t start)
    {
        auto& sum = Base::sum_;

        int64_t offset = Base::target_ - sum;

        int32_t size = seq != nullptr? seq->size() : 0;

        if (start + offset < size)
        {
            sum += offset;

            rank1_ += seq->rank(start >= 0 ? start : 0, start + offset, 1);

            return start + offset;
        }
        else {
            sum += (size - start);

            rank1_ += seq->rank(start >= 0 ? start : 0, seq->size(), 1);

            return size;
        }
    }

    int64_t finish(Iterator& iter, int32_t idx)
    {
        iter.local_pos() = idx;

        iter.iter_cache().add(this->sum_, rank1_);

        return this->sum_;
    }
};

template <typename Types>
class SkipBackwardWalker: public bt::FindBackwardWalkerBase<Types, SkipBackwardWalker<Types>> {
    typedef bt::FindBackwardWalkerBase<Types, SkipBackwardWalker<Types>>        Base;
    typedef typename Base::Key                                                  Key;

    int64_t rank1_ = 0;

public:
    typedef typename Base::ResultType                                           ResultType;
    typedef typename Base::Iterator                                             Iterator;

    SkipBackwardWalker(int32_t stream, int32_t index, Key target): Base(stream, index, target)
    {
        Base::search_type_ = SearchType::GT;
    }

    template <int32_t Idx, typename Tree>
    ResultType stream(const Tree* tree, int32_t start) {
        return Base::template stream<Idx>(tree, start);
    }

    template <int32_t StreamIdx, typename StreamType, typename Result>
    void postProcessStream(const StreamType* stream, int32_t start, const Result& result)
    {
        if (result.local_pos() >= 0)
        {
            rank1_ += stream->sum(2, result.local_pos() + 1, start + 1);
        }
        else {
            rank1_ += stream->sum(2, 0, start + 1);
        }
    }


    template <int32_t Idx, typename TreeTypes>
    ResultType stream(const PkdFSSeq<TreeTypes>* seq, int32_t start)
    {
        int64_t offset = Base::target_ - Base::sum_;

        auto& sum = Base::sum_;

        if (start - offset >= 0)
        {
            sum += offset;

            rank1_ += seq->rank(start - offset, start, 1);

            return start - offset;
        }
        else {
            sum += start;

            rank1_ += seq->rank(0, start, 1);

            return -1;
        }
    }

    int64_t finish(Iterator& iter, int32_t idx)
    {
        iter.local_pos() = idx;

        if (idx >= 0)
        {
            iter.iter_cache().sub(this->sum_, rank1_);
        }
        else {
            iter.iter_cache().setup(-1, 0);
        }

        return this->sum_;
    }
};





}
}}