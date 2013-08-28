
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <typeinfo>
#include <iostream>


#include <memoria/core/packed/packed_fse_tree.hpp>
#include <memoria/core/packed/packed_allocator.hpp>

using namespace std;
using namespace memoria;


int main(void) {

    Int SIZE = 1024;

    void* buf = malloc(SIZE);
    memset(buf, 0, SIZE);

    PackedAllocator* alloc = T2T<PackedAllocator*>(buf);
    alloc->init(SIZE, 1);

    typedef PkdFTreeTypes<Int, Int, Int, 2>     Types;
    typedef PkdFTree<Types>                 Tree;

    Int client_area =  alloc->client_area();

    Tree* tree = alloc->allocate<Tree>(0, client_area);

    tree->insertSpace(0, tree->max_size());

    for (Int c = 0; c < tree->size(); c++)
    {
        tree->value(0, c) = 1;
        tree->value(1, c) = 2;
    }

    tree->reindex();
    //tree->dump();

//  auto result = tree->findLTForward(0, 1, 0);
    auto result = tree->findLEBackward(1, 2, 6);

    cout<<result.idx()<<" "<<result.prefix()<<endl;



    return 0;
}

