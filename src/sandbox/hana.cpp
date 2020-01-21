
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


#include <memoria/profiles/default/default.hpp>
#include <memoria/core/datatypes/datatypes.hpp>

#include <memoria/core/types/hana.hpp>
#include <memoria/core/types/mp11.hpp>

#include <memoria/core/types/algo/select.hpp>

#include <memoria/core/tools/arena_buffer.hpp>

#include <memoria/core/tools/time.hpp>


#include <iostream>


using namespace memoria;

constexpr psize_t make_mask_for_stride(psize_t stride_log2)
{
    return (static_cast<psize_t>(1) << (stride_log2 - 1)) - 1;
}

constexpr psize_t compute_stride(psize_t stride_log2)
{
    return static_cast<psize_t>(1) << stride_log2;
}

using Value = psize_t;

psize_t locate(psize_t stride_log2_ex, const std::vector<Value>& values, Value element)
{
    psize_t size = values.size();
    psize_t basic_stride_log2 = stride_log2_ex;
    psize_t basic_stride = compute_stride(basic_stride_log2);

    psize_t stride_base = 0;
    if (MMA1_LIKELY(basic_stride <= size))
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

                if (MMA1_LIKELY(element < values[row]))
                {
                    break;
                }
                else if (element == values[row]) {
                    return row;
                }
                else {
                    stride_base = row + 1;
                }
            }

            stride >>= basic_stride_log2;
            stride_log2 -= basic_stride_log2;

            if (MMA1_LIKELY(c < strides)) {
                strides = basic_stride;
            }
            else {
                strides = (size - stride_base) >> stride_log2;
            }
        }
    }

    psize_t pos = PkdNotFound;

    for (psize_t c = stride_base; c < size; c++) {
        if (element <= values[c]) {
            return c;
        }
    }

    return pos;
}


int main()
{

    std::vector<Value> values;
    std::vector<psize_t> values_idx;

    for (psize_t c = 0; c < 35; c++)
    {
        values.push_back(c);
        values_idx.push_back(c);
    }

    std::random_shuffle(values_idx.begin(), values_idx.end());

    int64_t t0 = getTimeInMillis();


    for (psize_t c = 0; c < values_idx.size(); c++)
    {
//        std::cout << c << std::endl;
//        psize_t pos = locate(3, values, values_idx[c]);


        auto ii = std::upper_bound(values.begin(), values.end(), c);

        std::cout << c << " :: " << *ii << std::endl;

//        if (pos == PkdNotFound) {
//            std::cout << c << " :: " << pos << std::endl;
//        }
    }

    int64_t t1 = getTimeInMillis();

    std::cout << "Time: " << (t1 - t0) << std::endl;

    return 0;
}
