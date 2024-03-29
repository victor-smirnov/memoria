
// Copyright 2014 Victor Smirnov
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

#include <memoria/containers/sequence/sequence_cr_api.hpp>
#include <memoria/containers/sequence/sequence_cw_api.hpp>

#include <memoria/containers/sequence/sequence_chunk_impl.hpp>

#include <memoria/prototypes/bt_ss/btss_factory.hpp>

#include <memoria/core/packed/packed.hpp>

#include <memoria/core/strings/string.hpp>
#include <memoria/core/strings/string_codec.hpp>
#include <memoria/core/bytes/bytes_codec.hpp>

#include <memoria/core/tools/uuid.hpp>

#include <memoria/api/sequence/sequence_api.hpp>

namespace memoria {

template <
    typename Profile,
    size_t AlphabetSize_
>
struct BTTypes<Profile, Sequence<AlphabetSize_>>:
        public BTTypes<Profile, BTSingleStream>,
        public ICtrApiTypes<Sequence<AlphabetSize_>, Profile>
{
    using Base = BTTypes<Profile, BTSingleStream>;

    static constexpr size_t AlphabetSize = AlphabetSize_;
    static constexpr size_t BitsPerSymbol = BitsPerSymbolConstexpr(AlphabetSize);

    using CommonContainerPartsList = MergeLists<
                typename Base::CommonContainerPartsList,
                sequence::CtrRApiName
    >;

    using RWCommonContainerPartsList = MergeLists<
                typename Base::RWCommonContainerPartsList,
                sequence::CtrWApiName
    >;

    using LeafKeyStruct = PkdSSRLESeqT<AlphabetSize, 256, true>;

    using StreamDescriptors = TL<
            bt::StreamTF<
                TL<
                    TL<StreamSize>,
                    TL<LeafKeyStruct>
                >,
                bt::DefaultBranchStructTF
            >
    >;
};


template <typename Profile, size_t AlphabetSize, typename T>
class CtrTF<Profile, Sequence<AlphabetSize>, T>: public CtrTF<Profile, BTSingleStream, T> {
};



}
