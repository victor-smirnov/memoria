
// Copyright 2022 Victor Smirnov
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

#include <memoria/api/common/ctr_api.hpp>
#include <memoria/api/common/ctr_batch_input.hpp>

#include <memoria/core/datatypes/buffer/buffer.hpp>
#include <memoria/core/datatypes/buffer/ssrle_buffer.hpp>


namespace memoria {

template <size_t StreamIdx, size_t SubstreamIdx, typename DT>
decltype(auto) get_ctr_batch_input_substream(const DataTypeBuffer<DT>& input) {
    return input;
}

template <size_t StreamIdx, size_t SubstreamIdx, size_t AlphabetSize>
decltype(auto) get_ctr_batch_input_substream(const IOSSRLEBufferImpl<AlphabetSize>& input) {
    return input;
}

template <typename DT>
void clear_ctr_batch_input(DataTypeBuffer<DT>& input) {
    input.clear();
}

template <size_t AlphabetSize>
void clear_ctr_batch_input(IOSSRLEBufferImpl<AlphabetSize>& input) {
    input.clear();
}


template <typename DT>
void reindex_ctr_batch_input(DataTypeBuffer<DT>& input) {
    input.reindex();
}

template <size_t AlphabetSize>
void reindex_ctr_batch_input(IOSSRLEBufferImpl<AlphabetSize>& input) {
    input.reindex();
}



template <typename Streams>
auto ctr_batch_input_size_btss(const CtrBatchInputBase<Streams>& input) {
    return input.template get<0, 0>().size();
}

template <size_t AlphabetSize>
auto ctr_batch_input_size_btss(const IOSSRLEBufferImpl<AlphabetSize>& input) {
    return input.size();
}

template <typename DT>
auto ctr_batch_input_size_btss(const DataTypeBuffer<DT>& input) {
    return input.size();
}




}
