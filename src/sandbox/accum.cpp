
// Copyright 2015 Victor Smirnov
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


#include <memoria/v1/core/types/list/linearize.hpp>
#include <memoria/v1/prototypes/bt/tools/bt_tools.hpp>



#include <iostream>

using namespace memoria;
using namespace v1::bt;
using namespace v1::list_tree;
using namespace std;

class T{};

template <typename T, int32_t Indexes_>
struct PkdStruct {
    static const int32_t Indexes = Indexes_;
};

using BranchStructList = TypeList<
        PkdStruct<int32_t, 9>,

        PkdStruct<int32_t, 17>,

        PkdStruct<int32_t, 4>,

        PkdStruct<int32_t, 4>
>;


using LeafStructList = TypeList<
        TL<PkdStruct<int32_t, 4>, PkdStruct<int32_t, 2>, PkdStruct<int32_t, 2>>,

        PkdStruct<int32_t, 17>,

        PkdStruct<int32_t, 4>,

        PkdStruct<int32_t, 4>
>;

using IdxList = TypeList<
        TL<
            TL<IndexRange<0, 1>, IndexRange<2, 4>>,
            TL<IndexRange<0, 2>>,
            TL<IndexRange<0, 2>>
        >,
        TL<IndexRange<0, 3>, IndexRange<3, 5>, IndexRange<6, 12>, IndexRange<13, 17>>,
        TL<>,
        TL<IndexRange<0, 1>, IndexRange<1, 2>>
>;


using RangeListType = BranchNodeRangeListBuilder<
        BranchStructList,
        LeafStructList,
        IdxList
>::Type;

using RangeOffsetListType = BranchNodeRangeListBuilder<
        BranchStructList,
        LeafStructList,
        IdxList
>::OffsetList;


using AccType = IteratorBranchNodeEntryBuilder<
        BranchStructList,
        RangeListType
>::Type;


using AccumTuple = TupleBuilder<Linearize<AccType>>::Type;

int main() {
    ListPrinter<RangeListType>::print(cout);
    cout<<"Accum:"<<endl;
    ListPrinter<AccType>::print(cout);
    cout<<"AccumTuple:"<<endl;
    ListPrinter<TL<AccumTuple>>::print(cout);

    cout<<"LeafRangeList:"<<endl;
    ListPrinter<TL<IdxList>>::print(cout);


    using AccumItemH = AccumItem<TL<LeafStructList>, IntList<0, 0>, AccumTuple>;

    cout<<"RangeOffsetList:"<<endl;
    TypesPrinter<
        RangeOffsetListType,
        IntValue<AccumItemH::BranchIndex>
    >::print(cout);



    AccumTuple accum;

    try {
        AccumItemH::template item<6>(accum)[0] = 123;

        TypesPrinter<decltype(AccumItemH::template item<6>(accum))>::print(cout);

        int32_t index = 13;
        AccumItemH::value(index, accum) = 12345;
        cout<<"AccumItem = "<<AccumItemH::value(index, accum)<<endl;
    }
    catch (BoundsException& ex) {
        cout<<ex.message()<<endl;
    }

    using Tuple = std::tuple<int, int, int>;

    cout<<StreamTupleHelper<Tuple>::convert()<<endl;

    cout<<StreamTupleHelper<Tuple>::convertTupleAll(std::make_tuple(1, 5, 6.6))<<endl;



    using LeafStructList2 = TL<

            TypeList<
                TL<PkdStruct<int32_t, 4>, PkdStruct<int32_t, 2>>,

                PkdStruct<int32_t, 10>,

                TL<PkdStruct<int32_t, 4>, PkdStruct<int32_t, 2>>,

                PkdStruct<int32_t, 11>,

                TL<PkdStruct<int32_t, 4>, PkdStruct<int32_t, 2>, PkdStruct<int32_t, 2>>,

                PkdStruct<int32_t, 17>,

                PkdStruct<int32_t, 4>,

                PkdStruct<int32_t, 4>
            >,

            TypeList<
                PkdStruct<int32_t, 11>, //12 //br:8

                TL<PkdStruct<int32_t, 4>, PkdStruct<int32_t, 2>>,

                PkdStruct<int32_t, 11>,

                TL<PkdStruct<int32_t, 4>, PkdStruct<int32_t, 2>, PkdStruct<int32_t, 2>>,

                PkdStruct<int32_t, 17>
            >
    >;

    const int32_t LeafIdx = 13;

    cout<<"BranchIndex: "<<LeafToBranchIndexByValueTranslator<LeafStructList2, LeafIdx>::BranchStructIdx<<endl;
    cout<<"Offset: "<<LeafToBranchIndexByValueTranslator<LeafStructList2, LeafIdx>::LeafOffset<<endl;
    cout<<"ISStart: "<<LeafToBranchIndexByValueTranslator<LeafStructList2, LeafIdx>::IsStreamStart<<endl;
}
