
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_TESTS_SHARED_BTREE_TEST_BASE_HPP_
#define MEMORIA_TESTS_SHARED_BTREE_TEST_BASE_HPP_

#include <memoria/memoria.hpp>

#include <memoria/tools/tests.hpp>
#include <memoria/tools/tools.hpp>



namespace memoria {

template <
    typename ContainerTypeName,
    typename MemBuffer
>
class BTreeBatchTestBase: public SPTestTask {

    typedef BTreeBatchTestBase<
    			ContainerTypeName,
    			MemBuffer
    >                            														MyType;

protected:
    typedef typename SCtrTF<ContainerTypeName>::Type                                    Ctr;
    typedef typename Ctr::Iterator                                                      Iterator;
    typedef typename Ctr::Accumulator                                                   Accumulator;
    typedef typename Ctr::ID                                                            ID;

    Int max_block_size_     = 1024*40;

    Int page_size_cnt_      = 0;
    Int new_page_size_      = 0;

    Int ctr_name_;

    String dump_name_;

    BigInt  data_;
    bool    insert_;
    Int     block_size_;
    Int     page_step_;
    BigInt  pos_;
    Int     cnt_                = 0;
    Int     step_               = 0;


public:

    BTreeBatchTestBase(StringRef name):
        SPTestTask(name)
    {
        size_ = 1024*1024*16;

        MEMORIA_ADD_TEST_PARAM(max_block_size_);
        MEMORIA_ADD_TEST_PARAM(cnt_);
        MEMORIA_ADD_TEST_PARAM(page_size_cnt_);

        MEMORIA_ADD_TEST_PARAM(ctr_name_)->state();
        MEMORIA_ADD_TEST_PARAM(dump_name_)->state();

        MEMORIA_ADD_TEST_PARAM(data_)->state();
        MEMORIA_ADD_TEST_PARAM(insert_)->state();
        MEMORIA_ADD_TEST_PARAM(block_size_)->state();

        MEMORIA_ADD_TEST_PARAM(page_step_)->state();
        MEMORIA_ADD_TEST_PARAM(pos_)->state();
        MEMORIA_ADD_TEST_PARAM(cnt_)->state();
        MEMORIA_ADD_TEST_PARAM(step_)->state();

        MEMORIA_ADD_TEST_PARAM(new_page_size_)->state();

        MEMORIA_ADD_TEST_WITH_REPLAY(runTest, runReplay);
    }

    virtual ~BTreeBatchTestBase() throw() {}

    virtual MemBuffer createBuffer(Ctr& array, Int size, BigInt value)  = 0;
    virtual Iterator seek(Ctr& array, BigInt pos)                       = 0;
    virtual void insert(Iterator& iter, MemBuffer& data)                = 0;
    virtual void read(Iterator& iter, MemBuffer& data)                  = 0;
    virtual void remove(Iterator& iter, BigInt size)                    = 0;
    virtual void skip(Iterator& iter, BigInt offset)                    = 0;
    virtual BigInt getPosition(Iterator& iter)                          = 0;
    virtual BigInt getLocalPosition(Iterator& iter)                     = 0;
    virtual BigInt getSize(Ctr& array)                                  = 0;

    void runReplay(ostream& out)
    {
        Allocator allocator;
        LoadAllocator(allocator, dump_name_);

        check(allocator, "Allocator check failed",  MEMORIA_SOURCE);

        Ctr dv(&allocator, CTR_FIND, ctr_name_);

        dv.setNewPageSize(new_page_size_);

        if (insert_)
        {
            Build(out, allocator, dv);
        }
        else {
            remove(out, allocator, dv);
        }
    }


    virtual BigInt getRandomPosition(Ctr& array)
    {
        BigInt size = getSize(array);
        return getBIRandom(size);
    }

    virtual void setUp(ostream& out)
    {
        if (btree_random_branching_)
        {
            btree_branching_ = 8 + getRandom(100);
            out<<"BTree Branching: "<<btree_branching_<<endl;
        }
    }

    void runTest(ostream& out)
    {
        for (Int step = 0; step < 2; step++)
        {
            step_ = step;
            Run1(out, false);
        }

        // Run() will use different step for each ByteArray update operation
        Run1(out, true);
    }

    void Run1(ostream& out, bool step)
    {
        DefaultLogHandlerImpl logHandler(out);

        Allocator allocator;
        allocator.getLogger()->setHandler(&logHandler);

        Ctr dv(&allocator);

        ctr_name_ = dv.name();

        allocator.commit();

        dv.setBranchingFactor(btree_branching_);

        try {
            out<<"insert data"<<endl;
            insert_ = true;
            data_ = 1;//0xabcdef00;

            while (getSize(dv) < size_)
            {
                if (step)
                {
                    step_        = getRandom(3);
                }

                if (!isReplayMode())
                {
                    new_page_size_ = 4096 + getRandom(10)*1024;
                }

                dv.setNewPageSize(new_page_size_);

                block_size_  = 1 + getRandom(max_block_size_);

                Build(out, allocator, dv);

                allocator.commit();

                data_++;

                pos_         = -1;
                page_step_   = -1;

                page_size_cnt_++;
            }

            out<<"remove data. Sumset contains "<<(getSize(dv)/1024)<<"K keys"<<endl;
            insert_ = false;

            for (Int c = 0; ; c++)
            {
                if (step)
                {
                    step_ = getRandom(3);
                }

                BigInt size = getSize(dv);
                BigInt max_size = max_block_size_ <= size ? max_block_size_ : size;

                block_size_  = 1 + getBIRandom(max_size);
                page_step_   = getRandom(3);

                if (!remove(out, allocator, dv))
                {
                    break;
                }

                pos_         = -1;
                page_step_   = -1;

                allocator.commit();

                page_size_cnt_++;
            }

            out<<"Sumset.size = "<<(getSize(dv) / 1024)<<"K keys"<<endl;

            allocator.commit();
        }
        catch (...)
        {
            dump_name_ = Store(allocator);
            throw;
        }
    }

    void Build(ostream& out, Allocator& allocator, Ctr& array)
    {
        MemBuffer data = createBuffer(array, block_size_, data_);

        BigInt size = getSize(array);

        if (size == 0)
        {
            //insert buffer into an empty array
            auto iter = seek(array, 0);
            checkIterator(out, iter, MEMORIA_SOURCE);

            insert(iter, data);

            checkIterator(out, iter, MEMORIA_SOURCE);

            check(allocator, "insertion into an empty array failed. See the dump for details.", MEMORIA_SOURCE);

            auto iter1 = seek(array, 0);
            checkBufferWritten(iter1, data, "Failed to read and compare buffer from array", MEMORIA_SOURCE);
        }
        else {
            if (step_ == 0)
            {
                //insert at the start of the array
                auto iter = seek(array, 0);
                checkIterator(out, iter, MEMORIA_SOURCE);

                BigInt len = getSize(array);
                if (len > 100) len = 100;

                MemBuffer postfix = createBuffer(array, len, 0);

                read(iter, postfix);

                checkIterator(out, iter, MEMORIA_SOURCE);

                skip(iter, -len);
                checkIterator(out, iter, MEMORIA_SOURCE);

                insert(iter, data);
                checkIterator(out, iter, MEMORIA_SOURCE);

                check(allocator, "insertion at the start of the array failed. See the dump for details.",   MEMORIA_SOURCE);

                skip(iter, -data.size());
                checkIterator(out, iter, MEMORIA_SOURCE);

                checkBufferWritten(iter, data, "Failed to read and compare buffer from array",              MEMORIA_SOURCE);
                checkIterator(out, iter, MEMORIA_SOURCE);

                checkBufferWritten(iter, postfix, "Failed to read and compare buffer postfix from array",   MEMORIA_SOURCE);

                checkIterator(out, iter, MEMORIA_SOURCE);
            }
            else if (step_ == 1)
            {
                //insert at the end of the array
                BigInt len = getSize(array);

                auto iter = seek(array, len);
                checkIterator(out, iter, MEMORIA_SOURCE);

                if (len > 100) len = 100;

                MemBuffer prefix = createBuffer(array, len, 0);
                skip(iter, -len);
                checkIterator(out, iter, MEMORIA_SOURCE);

                read(iter, prefix);
                checkIterator(out, iter, MEMORIA_SOURCE);

                insert(iter, data);
                checkIterator(out, iter, MEMORIA_SOURCE);

                check(allocator, "insertion at the end of the array failed. See the dump for details.", MEMORIA_SOURCE);

                skip(iter, -data.size() - len);
                checkIterator(out, iter, MEMORIA_SOURCE);

                checkBufferWritten(iter, prefix, "Failed to read and compare buffer prefix from array", MEMORIA_SOURCE);
                checkIterator(out, iter, MEMORIA_SOURCE);

                checkBufferWritten(iter, data, "Failed to read and compare buffer from array",          MEMORIA_SOURCE);
                checkIterator(out, iter, MEMORIA_SOURCE);
            }
            else {
                //insert in the middle of the array

                if (pos_ == -1) pos_ = getRandomPosition(array);

                Int pos = pos_;

                auto iter = seek(array, pos);
                checkIterator(out, iter, MEMORIA_SOURCE);

                if (page_step_ == -1) page_step_ = getRandom(2);

                if (page_step_ == 0)
                {
                    skip(iter, -getLocalPosition(iter));
                    checkIterator(out, iter, MEMORIA_SOURCE);
                    pos = getPosition(iter);
                }

                BigInt prefix_len = pos;
                if (prefix_len > 100) prefix_len = 100;

                BigInt postfix_len = getSize(array) - pos;
                if (postfix_len > 100) postfix_len = 100;

                MemBuffer prefix    = createBuffer(array, prefix_len, 0);
                MemBuffer postfix   = createBuffer(array, postfix_len, 0);

                skip(iter, -prefix_len);
                checkIterator(out, iter, MEMORIA_SOURCE);

                read(iter, prefix);
                checkIterator(out, iter, MEMORIA_SOURCE);

                read(iter, postfix);
                checkIterator(out, iter, MEMORIA_SOURCE);

                skip(iter, -postfix_len);
                checkIterator(out, iter, MEMORIA_SOURCE);

                insert(iter, data);
                checkIterator(out, iter, MEMORIA_SOURCE);

                check(allocator, "insertion at the middle of the array failed. See the dump for details.",  MEMORIA_SOURCE);

                skip(iter, - data.size() - prefix_len);
                checkIterator(out, iter, MEMORIA_SOURCE);

                checkBufferWritten(iter, prefix,    "Failed to read and compare buffer prefix from array",  MEMORIA_SOURCE);
                checkIterator(out, iter, MEMORIA_SOURCE);

                checkBufferWritten(iter, data,      "Failed to read and compare buffer from array",         MEMORIA_SOURCE);
                checkIterator(out, iter, MEMORIA_SOURCE);

                checkBufferWritten(iter, postfix,   "Failed to read and compare buffer postfix from array", MEMORIA_SOURCE);
                checkIterator(out, iter, MEMORIA_SOURCE);
            }
        }
    }

    bool remove(ostream& out, Allocator& allocator, Ctr& array)
    {
        cnt_++;

        if (getSize(array) < 200)
        {
            auto iter = array.Begin();
            checkIterator(out, iter, MEMORIA_SOURCE);

            remove(iter, getSize(array));
            checkIterator(out, iter, MEMORIA_SOURCE);

            check(allocator, "remove ByteArray", MEMORIA_SOURCE);
            return getSize(array) > 0;
        }
        else {
            BigInt size = block_size_;

            if (step_ == 0)
            {
                //remove at the start of the array
                auto iter = seek(array, 0);
                checkIterator(out, iter, MEMORIA_SOURCE);

                BigInt len = getSize(array) - size;
                if (len > 100) len = 100;

                MemBuffer postfix(len);
                skip(iter, size);
                checkIterator(out, iter, MEMORIA_SOURCE);

                read(iter, postfix);
                checkIterator(out, iter, MEMORIA_SOURCE);

                skip(iter, -len - size);
                checkIterator(out, iter, MEMORIA_SOURCE);

                remove(iter, size);

                checkIterator(out, iter, MEMORIA_SOURCE);

                check(allocator, "Removing region at the start of the array failed. See the dump for details.", MEMORIA_SOURCE);

                checkBufferWritten(iter, postfix, "Failed to read and compare buffer postfix from array",       MEMORIA_SOURCE);
                checkIterator(out, iter, MEMORIA_SOURCE);
            }
            else if (step_ == 1)
            {
                //remove at the end of the array
                auto iter = seek(array, getSize(array) - size);
                checkIterator(out, iter, MEMORIA_SOURCE);

                BigInt len = getPosition(iter);
                if (len > 100) len = 100;

                MemBuffer prefix(len);
                skip(iter, -len);
                checkIterator(out, iter, MEMORIA_SOURCE);

                read(iter, prefix);
                checkIterator(out, iter, MEMORIA_SOURCE);

                remove(iter, size);
                checkIterator(out, iter, MEMORIA_SOURCE);

                check(allocator, "Removing region at the end of the array failed. See the dump for details.",   MEMORIA_SOURCE);

                skip(iter, -len);
                checkIterator(out, iter, MEMORIA_SOURCE);

                checkBufferWritten(iter, prefix, "Failed to read and compare buffer prefix from array",         MEMORIA_SOURCE);
                checkIterator(out, iter, MEMORIA_SOURCE);
            }
            else {
                //remove at the middle of the array
                if (pos_ == -1) pos_ = getRandomPosition(array);

                Int pos = pos_;

                auto iter = seek(array, pos);

                if (page_step_ == -1) page_step_ = getRandom(2);

                if (page_step_ == 0)
                {
                    skip(iter, -getLocalPosition(iter));
                    checkIterator(out, iter, MEMORIA_SOURCE);

                    pos = getPosition(iter);
                }

                if (pos + size > getSize(array))
                {
                    size = getSize(array) - pos - 1;
                }

                BigInt prefix_len = pos;
                if (prefix_len > 100) prefix_len = 100;

                BigInt postfix_len = getSize(array) - (pos + size);
                if (postfix_len > 100) postfix_len = 100;

                MemBuffer prefix(prefix_len);
                MemBuffer postfix(postfix_len);

                skip(iter, -prefix_len);
                checkIterator(out, iter, MEMORIA_SOURCE);

                read(iter, prefix);
                checkIterator(out, iter, MEMORIA_SOURCE);

                skip(iter, size);
                checkIterator(out, iter, MEMORIA_SOURCE);

                read(iter, postfix);
                checkIterator(out, iter, MEMORIA_SOURCE);

                skip(iter, -postfix_len - size);
                checkIterator(out, iter, MEMORIA_SOURCE);

                remove(iter, size);
                checkIterator(out, iter, MEMORIA_SOURCE);

                check(
                    allocator,
                    "Removing region at the middle of the array failed. See the dump for details.",
                    MEMORIA_SOURCE);

                skip(iter, -prefix_len);
                checkIterator(out, iter, MEMORIA_SOURCE);

                checkBufferWritten(
                        iter,
                        prefix,
                        "Failed to read and compare buffer prefix from array",
                        MEMORIA_SOURCE);

                checkIterator(out, iter, MEMORIA_SOURCE);

                checkBufferWritten(
                        iter,
                        postfix,
                        "Failed to read and compare buffer postfix from array",
                        MEMORIA_SOURCE);

                checkIterator(out, iter, MEMORIA_SOURCE);
            }

            return getSize(array) > 0;
        }

        return false;
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
                                                          <<" idx="<<idx
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

        if (iter.prefix(0) != prefix.key(0))
        {
            iter.dump(out);
            throw TestException(source, SBuf()<<"Invalid prefix value. Iterator: "<<iter.prefix()<<" Actual: "<<prefix);
        }
    }

};

}


#endif
