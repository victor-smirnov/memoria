
// Copyright 2019 Victor Smirnov
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


#include <memoria/core/types.hpp>


namespace memoria {

// N-Ary search routine

template <typename ProviderFn, typename Value>
psize_t locate(psize_t stride_log2_ex, psize_t size, ProviderFn&& values, Value element)
{
    psize_t basic_stride_log2 = stride_log2_ex;
    psize_t basic_stride = static_cast<psize_t>(1) << basic_stride_log2;

    psize_t stride_base = 0;
    if (MMA_LIKELY(basic_stride <= size))
    {
        psize_t stride = basic_stride;
        psize_t stride_log2 = basic_stride_log2;

        for (
             psize_t stride_tmp = basic_stride, stride_log2_tmp = basic_stride_log2;
             stride_tmp < size;
             stride_tmp <<= basic_stride_log2, stride_log2_tmp += basic_stride_log2)
        {
            stride = stride_tmp;
            stride_log2 = stride_log2_tmp;
        }

        psize_t strides = size >> stride_log2;

        while (stride >= basic_stride)
        {
            psize_t c;
            for (c = 0; c < strides; c++)
            {
                psize_t row = stride_base + stride - 1;

                Value vv = values(row);

                if (MMA_LIKELY(element < vv))
                {
                    break;
                }
                else if (element == vv) {
                    return row;
                }
                else {
                    stride_base = row + 1;
                }
            }

            stride >>= basic_stride_log2;
            stride_log2 -= basic_stride_log2;

            if (MMA_LIKELY(c < strides)) {
                strides = basic_stride;
            }
            else {
                strides = (size - stride_base) >> stride_log2;
            }
        }
    }

    psize_t pos = PkdNotFound;

    for (psize_t c = stride_base; c < size; c++) {
        if (element <= values(c)) {
            return c;
        }
    }

    return pos;
}

}
