// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_MAP_MAP_TEST_BASE_HPP_
#define MEMORIA_TESTS_MAP_MAP_TEST_BASE_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>


#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>

namespace memoria {

class MapTestBase: public SPTestTask {
    typedef MapTestBase MyType;

public:
    typedef KVPair<BigInt, BigInt> Pair;

protected:
    typedef vector<Pair> PairVector;
    typedef SmallCtrTypeFactory::Factory<Map1>::Type                    		Ctr;
    typedef typename Ctr::Iterator                               				Iterator;
    typedef typename Ctr::ID                                     				ID;
    typedef typename Ctr::Accumulator                            				Accumulator;

    PairVector pairs;
    PairVector pairs_sorted;

    Int vector_idx_;
    Int step_;

    BigInt ctr_name_;
    String dump_name_;
    String pairs_data_file_;
    String pairs_sorted_data_file_;

    bool throw_ex_ = false;

    public:

    MapTestBase(StringRef name): SPTestTask(name)
    {
        MEMORIA_ADD_TEST_PARAM(throw_ex_);

        MEMORIA_ADD_TEST_PARAM(vector_idx_)->state();
        MEMORIA_ADD_TEST_PARAM(step_)->state();

        MEMORIA_ADD_TEST_PARAM(ctr_name_)->state();
        MEMORIA_ADD_TEST_PARAM(dump_name_)->state();
        MEMORIA_ADD_TEST_PARAM(pairs_data_file_)->state();
        MEMORIA_ADD_TEST_PARAM(pairs_sorted_data_file_)->state();
    }

    virtual ~MapTestBase() throw () {
    }

    void checkContainerData(Ctr& map, PairVector& pairs)
    {
        Int pairs_size = (Int) pairs.size();

        Int idx = 0;
        for (auto iter = map.Begin(); !iter.isEnd();)
        {
            BigInt key   = iter.getKey(0);
            BigInt value = iter.getValue();

            MEMORIA_TEST_THROW_IF_1(pairs[idx].key_,   !=, key, idx);
            MEMORIA_TEST_THROW_IF_1(pairs[idx].value_, !=, value, idx);

            iter.next();
            idx++;
        }

        MEMORIA_TEST_THROW_IF   (idx, !=, pairs_size);

        idx = pairs_size - 1;
        for (auto iter = map.RBegin(); !iter.isBegin(); )
        {
            BigInt  key     = iter.getKey(0);
            BigInt  value   = iter.getValue();

            MEMORIA_TEST_THROW_IF_1(pairs[idx].key_,   !=, key, idx);
            MEMORIA_TEST_THROW_IF_1(pairs[idx].value_, !=, value, idx);

            iter.prev();

            idx--;
        }

        MEMORIA_TEST_THROW_IF_EXPR(idx != -1, idx, pairs_size);
    }

    void StorePairs(const PairVector& pairs, const PairVector& pairs_sorted)
    {
        String basic_name = "Data." + getName();

        String pairs_name = basic_name + ".pairs.txt";
        pairs_data_file_ = getResourcePath(pairs_name);

        StoreVector(pairs, pairs_data_file_);

        String pairs_sorted_name = basic_name + ".pairs_sorted.txt";
        pairs_sorted_data_file_ = getResourcePath(pairs_sorted_name);

        StoreVector(pairs_sorted, pairs_sorted_data_file_);
    }


    virtual void checkIterator(ostream& out, Iterator& iter, const char* source)
    {
        checkIteratorPrefix(out, iter, source);

        auto& path = iter.path();

        for (Int level = path.getSize() - 1; level > 0; level--)
        {
            bool found = false;

            for (Int idx = 0; idx < path[level]->children_count(); idx++)
            {
                ID id = iter.model().getINodeData(path[level].node(), idx);
                if (id == path[level - 1]->id())
                {
                    if (path[level - 1].parent_idx() != idx)
                    {
                        iter.dump(out);
                        throw TestException(source, SBuf()<<"Invalid parent-child relationship for node:"
                                                          <<path[level]->id()
                                                          <<" child: "
                                                          <<path[level - 1]->id()
                                                          <<" idx="
                                                          <<idx
                                                          <<" parent_idx="
                                                          <<path[level-1].parent_idx());
                    }
                    else {
                        found = true;
                        break;
                    }
                }
            }

            if (!found)
            {
                iter.dump(out);
                throw TestException(source, SBuf()<<"Child: "
                                                  <<path[level - 1]->id()
                                                  <<" is not fount is it's parent, parent_idx="
                                                  <<path[level - 1].parent_idx());
            }
        }


    }

    virtual void checkIteratorPrefix(ostream& out, Iterator& iter, const char* source)
    {
        Accumulator prefix;

        iter.ComputePrefix(prefix);

        if (iter.prefix() != prefix.key(0))
        {
            iter.dump(out);
            throw TestException(source, SBuf()<<"Invalid prefix value. Iterator: "<<iter.prefix()<<" Actual: "<<prefix);
        }
    }


    virtual void setUp(ostream& out)
    {
        if (btree_random_branching_)
        {
            btree_branching_ = 8 + getRandom(100);
            out<<"BTree Branching: "<<btree_branching_<<endl;
        }

        pairs.clear();
        pairs_sorted.clear();

        for (Int c = 0; c < size_; c++)
        {
            pairs.push_back(Pair(getUniqueBIRandom(pairs, 1000000), getBIRandom(100000)));
        }
    }
};

}

#endif