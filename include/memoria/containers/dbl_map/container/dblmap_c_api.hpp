
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_CONTAINER_DBLMAP_C_API_HPP
#define _MEMORIA_CONTAINER_DBLMAP_C_API_HPP


#include <memoria/containers/dbl_map/dblmap_names.hpp>
#include <memoria/containers/dbl_map/dblmap_tools.hpp>

#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

namespace memoria {

using namespace memoria::bt;

MEMORIA_CONTAINER_PART_BEGIN(memoria::dblmap::CtrApiName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Base::ID                                                   ID;

    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::NodeDispatcher                                       NodeDispatcher;
    typedef typename Base::RootDispatcher                                       RootDispatcher;
    typedef typename Base::LeafDispatcher                                       LeafDispatcher;
    typedef typename Base::NonLeafDispatcher                                    NonLeafDispatcher;


    typedef typename Base::Key                                                  Key;
    typedef typename Base::Value                                                Value;
    typedef typename Base::Element                                              Element;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::Accumulator                                         Accumulator;
    typedef typename Types::Position                                            Position;

    typedef typename Base::TreePath                                             TreePath;
    typedef typename Base::TreePathItem                                         TreePathItem;

    static const Int Streams                                                    = Types::Streams;

    typedef typename Types::DataSource                                     		DataSource;

    BigInt total_size() const
    {
        auto sizes = self().getTotalKeyCount();
        return sizes[1];
    }

    BigInt size() const
    {
        auto sizes = self().getTotalKeyCount();
        return sizes[0];
    }

    BigInt blob_size(Key id) const
    {
        return seek(id).size();
    }

    Iterator seek(Key id, Key pos)
    {
        Iterator iter = self().find(id);
        MEMORIA_ASSERT_TRUE(iter.found());

        iter.findData(pos);

        return iter;
    }

    Iterator find(Key id)
    {
        auto& self = this->self();

        dblmap::MapFindWalker<Types> walker(id);

        Iterator iter = self.find0(0, walker);

        if ((!iter.isEnd()) && iter.id() == id)
        {
            iter.found()    = true;
        }
        else {
            iter.found()    = false;
        }

        return iter;
    }

    bool contains(Key id) {
        return find(id).found();
    }

    BigInt maxId()
    {
        auto& self = this->self();

        Iterator iter = self.REnd();

        return iter.id();
    }

    Iterator create(const Key& id)
    {
        auto& self = this->self();
        auto iter  = self.find(id);

        if (!iter.found())
        {
        	EmptyDataSource<typename Types::IOValue> src;
        	self.insert(iter, id, src);
        	iter.found() = true;
        }

        return iter;
    }

    Iterator createNew(const Key& id)
    {
        auto& self = this->self();
        auto iter  = self.find(id);

        if (!iter.found())
        {
        	EmptyDataSource<typename Types::IOValue> src;
        	self.insert(iter, id, src);
        	iter.found() = true;
        }
        else {
        	throw vapi::Exception(MA_SRC, SBuf()<<"DmblMap Entry with key "<<id<<" already exists");
        }

        return iter;
    }


    bool remove(Key id)
    {
        auto& self = this->self();
        auto iter  = self.find(id);

        if (iter.found())
        {
            self.removeEntry(iter);

            return true;
        }
        else {
            return false;
        }
    }



MEMORIA_CONTAINER_PART_END

}


#endif
