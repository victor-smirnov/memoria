
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/v1/core/packed/tree/fse/packed_fse_quick_tree.hpp>
#include <memoria/v1/core/packed/tree/vle/packed_vle_quick_tree.hpp>
#include <memoria/v1/core/packed/tree/vle/packed_vle_dense_tree.hpp>



#include <memoria/v1/core/tools/i7_codec.hpp>
#include <memoria/v1/core/tools/elias_codec.hpp>
#include <memoria/v1/core/tools/time.hpp>
#include <memoria/v1/core/tools/random.hpp>

#include <memoria/v1/core/packed/tools/packed_struct_ptrs.hpp>

#include <iostream>

#include <cstddef>

using namespace memoria;
using namespace std;


//template <class Tp>
//struct SimpleAllocator {
//  typedef Tp value_type;
//  SimpleAllocator() {}
//
//  template <class T> SimpleAllocator(const SimpleAllocator<T>& other) {
//    cout<<"CopyAllocator: "<<endl;
//  }
//
//  Tp* allocate(std::size_t n) {
//
//    cout<<"Allocate: "<<n<<endl;
//
//    return T2T<Tp*>(malloc(sizeof(Tp) * n));
//  }
//
//  Tp* allocate(std::size_t n, const void* cvptr) {
//    cout<<"Allocate CVPTR: "<<n<<endl;
//    return nullptr;
//  }
//
//  void deallocate(Tp* p, std::size_t n) {
//    cout<<"DeAllocate: "<<n<<endl;
//
//    ::free(p);
//  }
//
//  template <typename Args>
//  void construct(Tp* xptr, Args&& args) {
//    cout<<"Construct "<<xptr<<endl;
//    *xptr = args;
//  }
//
//
//  void destroy(Tp* xptr) {
//    cout<<"Destroy "<<xptr<<endl;
//  }
//};
//template <class T, class U>
//bool operator==(const SimpleAllocator<T>&, const SimpleAllocator<U>&) {
//  return true;
//}
//
//template <class T, class U>
//bool operator!=(const SimpleAllocator<T>&, const SimpleAllocator<U>&) {
//  return false;
//}




int main() {

    try {

//      auto ptr0 = std::allocate_shared<Int>(SimpleAllocator<Int>(), 456);
//
//      auto ptr1 = std::make_shared<Int>(123);
//
//      cout<<"Value: "<<*ptr1<<endl;
//      cout<<"Value: "<<*ptr0<<endl;

        constexpr Int Block = 1;
        constexpr Int Blocks = Block + 1;

//      using Tree = PkdVQTreeT<BigInt, Blocks, UByteI7Codec>;
        using Tree = PkdVDTreeT<BigInt, Blocks, UByteI7Codec, BigInt, PackedTreeBranchingFactor, 128>;
//      using Tree = PkdFQTreeT<BigInt, Blocks>;

        using Values = Tree::Values;

        Int block_size = 4096*100000;

        auto tree = MakeSharedPackedStructByBlock<Tree>(block_size);

        Seed(1234);

        Int size = 1000;

        vector<Values> data(size);

        for (auto& v: data) v = Values{1 + getRandomG(300), 1 + getRandomG(300)};

        long t0 = getTimeInMillis();
        tree->_insert(0, size, [&](Int block, Int idx){return data[idx][block];});

        long t1 = getTimeInMillis();

//      tree->dump();


        long ts1 = getTimeInMillis();
        cout<<"----------------------Sums"<<endl;
//      for (Int c = 1; c < tree->size(); c += 1)
//      {
//          auto sum0 = tree->sum(0, c);
//          auto sum1 = tree->plain_sum(0, c);
//
//          if (sum0 != sum1)
//          {
//              cout<<c<<": "<<sum0<<"  "<<sum1<<endl;
//          }
//      }
        long ts2 = getTimeInMillis();


        auto max = tree->sum(Block, size);

        cout<<"MAX: "<<max<<endl;

        cout<<tree->sum(0, size)<<endl;
        cout<<tree->sum(1, size)<<endl;

        cout<<"find:    "<<tree->findGEForward(Block, 0, max).idx()<<endl;
        cout<<"find_bw: "<<tree->findGEBackward(Block, size - 1, max).idx()<<endl;


        long t2 = getTimeInMillis();

        cout<<"----------------------FindStart"<<endl;
        for (Int c = 0; c < size; c++)
        {
            auto key = tree->sum(Block, c + 1);
            auto idx = tree->find_ge(Block, key).idx();

            if (idx != c)
            {
                cout<<c<<": "<<key<<"  "<<idx<<endl;
            }
        }

        long t3 = getTimeInMillis();

        cout<<"----------------------FW"<<endl;
        for (Int c = 0; c < size; c++)
        {
            auto key = tree->sum(Block, c, size);
            auto idx = tree->find_ge_fw(Block, c, key).idx();

            if (idx != size - 1) {
                cout<<c<<": "<<key<<"  "<<idx<<endl;
            }
        }

        long t4 = getTimeInMillis();

        cout<<"----------------------BW"<<endl;
        for (Int c = tree->size() - 1; c >= 0; c--)
        {
            auto key = tree->sum(Block, c + 1);

            auto idx = tree->findGEBackward(Block, c, key).idx();
            if (idx != 0) {
                cout<<c<<": "<<key<<"  "<<idx<<endl;
            }
        }

        long t5 = getTimeInMillis();

        vector<BigInt> keys(10000000);
        for (auto& v: keys) v = getRandomG(max);

        long t6 = getTimeInMillis();

        for (auto k: keys)
        {
            auto idx = tree->find_ge(Block, k).idx();
            if (idx == size) {
                cout<<k<<"  "<<idx<<endl;
            }
        }

        long t7 = getTimeInMillis();


        vector<BigInt> keys_s(10000000);
        BigInt sum = 0;

        tree->scan(Block, 0, size, [&](Int c, auto value){
            keys_s[c] = sum + value;
            sum += value;
        });

        long t8 = getTimeInMillis();

        for (auto k: keys_s)
        {
            auto idx = tree->find_ge(0, k).idx();
            if (idx == size) {
                cout<<k<<"  "<<idx<<endl;
            }
        }

        long t9 = getTimeInMillis();


        cout<<"Insert: "<<FormatTime(t1 - t0)<<endl;
        cout<<"Sums: "<<FormatTime(ts2 - ts1)<<endl;
        cout<<"FindStart: "<<FormatTime(t3 - t2)<<endl;
        cout<<"FindFW: "<<FormatTime(t4 - t3)<<endl;
        cout<<"FindBW: "<<FormatTime(t5 - t4)<<endl;

        cout<<"FindRnd: "<<FormatTime(t7 - t6)<<endl;
        cout<<"FindSeq: "<<FormatTime(t9 - t8)<<endl;
    }
    catch (PackedOOMException& ex) {
        cout<<ex.source()<<endl;
        cout<<ex<<endl;
    }
    catch (Exception& ex) {
        cout<<ex.source()<<endl;
        cout<<ex<<endl;
    }
}



