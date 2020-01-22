
// Copyright 2012 Victor Smirnov
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


#include <memoria/tests/tests.hpp>
#include <memoria/tests/assertions.hpp>

#include <memoria/core/integer/big_accumulator.hpp>

#include <sstream>

namespace memoria {
namespace tests {

namespace {

class IntegerTestsSuite: public TestState {
    using MyType = IntegerTestsSuite;
    using Base = TestState;

    using Acc256T = UnsignedAccumulator<256>;
    using BmpInt256T = boost::multiprecision::uint256_t;

public:
    static void init_suite(TestSuite& suite) {
        MMA_CLASS_TESTS(suite, basic_tests, complex_tests);
    }

    std::string make_random_num_str(size_t max_size)
    {
        std::stringstream ss;

        size_t size = 1 + this->getRandom(max_size - 1);

        for (size_t c = 0; c < size; c++)
        {
            int d = this->getRandom(10);
            if (c == 0 && d == 0) {
                d = 1;
            }

            ss << d;
        }

        return ss.str();
    }

    void basic_tests()
    {
		Acc256T acc0{};
        BmpInt256T int0{};
        assert_equals(int0, acc0);

        Acc256T acc1{1};
        BmpInt256T int1{1};
        assert_equals(int1, acc1);

        std::string num_s = "1234567890123456789012345678901234567890";
        std::stringstream s1;
        Acc256T acc3{num_s};
        s1 << acc3;
        assert_equals(num_s, s1.str());

        Acc256T acc4;
        s1 >> acc4;
        assert_equals(acc3, acc4);

        std::string str_num5_1 = "115699999999999999999999999999999999999999999999999999999999999999999999999999";
        std::string str_num5_2 = "115700000000000000000000000000000000000000000000000000000000000000000000000000";

        Acc256T acc5{str_num5_1};
        acc5++;
        assert_equals(acc5.to_bmp().str(), str_num5_2);

        acc5--;
        assert_equals(acc5.to_bmp().str(), str_num5_1);
    }

    void complex_tests()
    {
        for (size_t c = 0; c < 100000; c++)
        {
            //69 digits max
            auto str = make_random_num_str(69);

            Acc256T acc(str);
            BmpInt256T bmp_int(str);

            assert_equals(acc.to_bmp(), bmp_int);
            assert_equals(acc.to_bmp().str(), str);

            check_numbers(acc, bmp_int);
        }
    }

    void check_numbers(Acc256T acc, BmpInt256T bmp_int)
    {
        assert_equals(acc.to_bmp(), bmp_int);
        assert_equals(acc, Acc256T{bmp_int});

        acc += 1;

        assert_equals(acc, Acc256T{bmp_int + 1});
        assert_equals(true, acc > Acc256T{bmp_int});
        assert_equals(true, acc >= Acc256T{bmp_int});
        assert_equals(false, acc < Acc256T{bmp_int});
        assert_equals(false, acc <= Acc256T{bmp_int});


        acc -= 2;

        assert_equals(acc, Acc256T{bmp_int - 1});
        assert_equals(false, acc > Acc256T{bmp_int});
        assert_equals(false, acc >= Acc256T{bmp_int});
        assert_equals(true, acc < Acc256T{bmp_int});
        assert_equals(true, acc <= Acc256T{bmp_int});

        acc += 1;

        auto acc2 = acc + acc;
        assert_equals(Acc256T{bmp_int + bmp_int}, acc2);

        assert_equals(Acc256T{0}, acc - acc);

        Acc256T acc0{};
        BmpInt256T bmp_acc0{};

        for (size_t c = 0; c < 100; c++)
        {
            acc0 += acc;
            bmp_acc0 += bmp_int;

            assert_equals(Acc256T{bmp_acc0}, acc0);
        }

        for (size_t c = 0; c < 100; c++)
        {
            acc0 -= acc;
            bmp_acc0 -= bmp_int;

            assert_equals(Acc256T{bmp_acc0}, acc0);
        }

        Acc256T acc1{acc};
        BmpInt256T bmp_acc1{bmp_int};

        for (size_t c = 0; c < 100; c++)
        {
            acc1 -= acc;
            bmp_acc1 -= bmp_int;

            assert_equals(Acc256T{bmp_acc1}, acc1);
        }
    }
};

auto Suite1 = register_class_suite<IntegerTestsSuite>("Integer");

}

}}
