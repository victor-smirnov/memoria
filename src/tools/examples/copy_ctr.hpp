// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_EXAMPLES_COPY_CTR_HPP_
#define MEMORIA_EXAMPLES_COPY_CTR_HPP_

#include "examples.hpp"

#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>

namespace memoria {

class CopyCtrExample: public SPExampleTask {
public:
    typedef KVPair<BigInt, BigInt>                                              Pair;

private:
    typedef vector<Pair> PairVector;
    typedef SCtrTF<VectorMap<BigInt, Byte>>::Type                               Ctr;
    typedef typename Ctr::Iterator                                              Iterator;
    typedef typename Ctr::ID                                                    ID;


    PairVector pairs;
    PairVector pairs_sorted;

public:

    CopyCtrExample() :
        SPExampleTask("CopyCtr")
    {}

    virtual ~CopyCtrExample() throw () {
    }

    Ctr createCtr(Allocator& allocator, BigInt name)
    {
        return Ctr(&allocator, name, true);
    }

    Ctr createCtr1(Allocator& allocator, BigInt name)
    {
        Ctr map = createCtr(allocator, name);

//        map[123456] = 10;

        return map;
    }


    virtual void Run(ostream& out)
    {
        DefaultLogHandlerImpl logHandler(out);

        {

            if (this->btree_random_airity_)
            {
                this->btree_branching_ = 8 + getRandom(100);
                out<<"BTree Branching: "<<this->btree_branching_<<endl;
            }

            Allocator allocator;
            allocator.getLogger()->setHandler(&logHandler);


            Ctr map(createCtr1(allocator, 1));

            cout<<"Map has been created"<<endl;

            Ctr map2 = map;

            cout<<"Map2 has been created as a copy of Map"<<endl;

            cout<<"About to reinitialize Map2"<<endl;

            map2 = createCtr(allocator, 2);

            cout<<"Map2 has been reinitialized"<<endl;

//            for (Int c = 1; c <= 10; c++)
//            {
//                map[c] = c;
//            }

            allocator.commit();

        }

        cout<<endl<<"Done. CtrCounters="<<CtrRefCounters<<" "<<CtrUnrefCounters<<endl<<endl;
    }
};

}

#endif
