
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CONTAINERS_DBLMAP2_I_API_HPP
#define MEMORIA_CONTAINERS_DBLMAP2_I_API_HPP

#include <memoria/core/container/container.hpp>

#include <memoria/containers/dbl_map/dblmap_names.hpp>

#include <functional>

namespace memoria {


MEMORIA_ITERATOR_PART_BEGIN(memoria::dblmap::ItrApi2Name)

    typedef Ctr<DblMap2CtrTypes<Types>>                         ContainerType;

    typedef typename ContainerType::OuterMap::Iterator          OuterMapIterator;
    typedef typename ContainerType::InnerMap::Iterator          InnerMapIterator;
    typedef typename ContainerType::Value                       Value;
    typedef typename ContainerType::Key                         Key;

    typedef typename ContainerType::InnerMap::Element           InnerMapElement;
    typedef typename ContainerType::InnerMap::Accumulator       InnerMapAccumulator;

    typedef typename ContainerType::OuterMap::Types::CtrSizeT   CtrSizeT;

    Value value() const
    {
        return self().inner_iter().getValue();
    }

    void setValue(const Value& value)
    {
        self().inner_iter().setValue(value);
    }

    Key id() const {
        return key();
    }

    Key key() const
    {
        return self().outer_iter().key();
    }

    Key key2() const
    {
        return self().inner_iter().key();
    }

    CtrSizeT global_pos() const
    {
        return self().inner_iter().pos();
    }

    CtrSizeT pos() const
    {
        CtrSizeT global_pos     = self().global_pos();
        CtrSizeT base           = self().outer_iter().base();

        return global_pos - base;
    }

    CtrSizeT blob_size() const {
        return size();
    }

    CtrSizeT size() const
    {
        return self().outer_iter().size();
    }

    bool findKeyGE(Key key)
    {
        auto& self = this->self();

        if (self.pos() > 0)
        {
            self.skipBw(self.pos());
        }

        MEMORIA_ASSERT(self.pos(), ==, 0);
        MEMORIA_WARNING(self.inner_iter().key_prefix(), !=, 0);

        if (self.inner_iter().findKeyGE(key))
        {
            CtrSizeT pos    = self.pos();
            CtrSizeT size = self.size();

            if (pos > size)
            {
                self.inner_iter().skipBw(pos - size);
                return false;
            }
            else {
                return pos < size;
            }
        }
        else {
            CtrSizeT pos    = self.pos();
            CtrSizeT size = self.size();

            if (pos > size)
            {
                self.inner_iter().skipBw(pos - size);
            }

            return false;
        }
    }

    bool findKeyGT(Key key)
    {
        auto& self = this->self();

        if (self.pos() > 0)
        {
            self.skipBw(self.pos());
        }

        MEMORIA_ASSERT(self.pos(), ==, 0);
        MEMORIA_WARNING(self.inner_iter().key_prefix(), !=, 0);

        if (self.inner_iter().findKeyGT(key))
        {
            CtrSizeT pos    = self.pos();
            CtrSizeT size = self.size();

            if (pos > size)
            {
                self.inner_iter().skipBw(pos - size);
                return false;
            }
            else {
                return true;
            }
        }
        else {
            CtrSizeT pos    = self.pos();
            CtrSizeT size = self.size();

            if (pos > size)
            {
                self.inner_iter().skipBw(pos - size);
            }

            return false;
        }
    }

    bool findKeyLE(Key key)
    {
        auto& self = this->self();

        if (self.findKeyGE(key))
        {
            Key current = self.inner_iter().key();

            if (current > key)
            {
                if (self.pos() > 0)
                {
                    self.skipBw(1);

                    return true;
                }
                else {
                    return false;
                }
            }
            else {
                return true;
            }
        }
        else if (self.pos() > 0)
        {
            self.inner_iter().skipBw(1);
            return true;
        }
        else {
            return false;
        }
    }

    bool find2ndLE(Key key)
    {
        auto& self = this->self();

        if (self.findKeyLE(key))
        {
            return self.key2() <= key;
        }
        else {
            return false;
        }
    }

    bool find2ndGE(Key key)
    {
        auto& self = this->self();

        if (self.findKeyGE(key))
        {
            return self.key2() >= key;
        }
        else {
            return false;
        }
    }

    bool find2ndGT(Key key)
    {
        auto& self = this->self();

        if (self.findKeyGT(key))
        {
            return self.key2() > key;
        }
        else {
            return false;
        }
    }

    bool find2ndEQ(Key key)
    {
        auto& self = this->self();

        if (self.findKeyGE(key))
        {
            return self.key2() == key;
        }
        else {
            return false;
        }
    }


    bool insert2nd(Key key, const Value& value) {
        return insert(key, value);
    }

    void updateKey2(Key amount)
    {
        auto& self = this->self();
        self.inner_iter().updateKey(amount);

        CtrSizeT pos    = self.pos();
        CtrSizeT size   = self.size();

        if (pos < size - 1)
        {
            self.skipFw(1);
            self.inner_iter().updateKey(-amount);
            self.skipBw(1);
        }
    }

    bool insert(Key key, const Value& value)
    {
        auto& self = this->self();

        InnerMapElement element;

        element.second = value;

        if (self.findKeyGE(key))
        {
            if (self.key2() != key)
            {
                CtrSizeT key_prefix = self.inner_iter().key_prefix();

                CtrSizeT key_delta = key - key_prefix;

                std::get<0>(element.first)[0] = 1;
                std::get<0>(element.first)[1] = key_delta;

                self.inner_iter().ctr().insertMapEntry(self.inner_iter(), element);

                self.inner_iter().updateKey(-key_delta);

                self.outer_iter().addSize(1);

                return true;
            }
            else {
                self.setValue(value);
                return false;
            }
        }
        else {
            CtrSizeT key_prefix = self.inner_iter().key_prefix();

            CtrSizeT key_delta = key - key_prefix;

            std::get<0>(element.first)[0] = 1;
            std::get<0>(element.first)[1] = key_delta;

            self.inner_iter().ctr().insertMapEntry(self.inner_iter(), element);

            self.outer_iter().addSize(1);
            return true;
        }
    }

    bool remove(Key key)
    {
        auto& self = this->self();

        if (self.pos() > 0)
        {
            self.skipBw(-self.pos());
        }

        if (self.findKeyGE(key))
        {
            if (self.key2() == key)
            {
                return self.remove();
            }
            else {
                return false;
            }
        }
        else {
            return false;
        }
    }

    CtrSizeT removeRange(CtrSizeT length)
    {
        auto& self = this->self();

        CtrSizeT pos    = self.pos();
        CtrSizeT size   = self.size();

        if (pos < size)
        {
            CtrSizeT to_remove = (pos + length <= size) ? length : size - pos;

            InnerMapAccumulator sums;

            for (CtrSizeT c = 0; c < to_remove; c++)
            {
                self.inner_iter().ctr().removeMapEntry(self.inner_iter(), sums);
            }

            if (pos + to_remove < size)
            {
                self.inner_iter().updateKey(std::get<0>(sums)[1]);
            }

            self.outer_iter().addSize(-to_remove);

            return to_remove;
        }
        else {
            return 0;
        }
    }


//    BigInt removeRange(BigInt length)
//    {
//      auto& self = this->self();
//
//      BigInt pos  = self.pos();
//      BigInt size = self.size();
//
//      if (pos < size)
//      {
//          BigInt to_remove = (pos + length <= size) ? length : size - pos;
//
//          auto to = self.inner_iter();
//          to.skipFw(to_remove);
//
//          InnerMapAccumulator sums;
//          self.inner_iter().ctr().removeMapEntries(self.inner_iter(), to, sums);
//
//          self.inner_iter() = to;
//
//          self.outer_iter().addSize(-to_remove);
//
//          if (pos + to_remove < size)
//          {
//              self.inner_iter().updateKey(std::get<0>(sums)[1]);
//          }
//
//          return to_remove;
//      }
//      else {
//          return 0;
//      }
//    }

    void removeEntry()
    {
        auto& self = this->self();

        if (self.pos() > 0)
        {
            self.skipBw(-self.pos());
        }

        CtrSizeT length = self.size();
        CtrSizeT removed    = self.removeRange(length);

        MEMORIA_ASSERT(removed, == , length);

        self.outer_iter().remove();
    }

    bool remove()
    {
        auto& self = this->self();

        CtrSizeT pos    = self.pos();
        CtrSizeT size   = self.size();

        if (pos < size)
        {
            InnerMapAccumulator sums;
            self.inner_iter().ctr().removeMapEntry(self.inner_iter(), sums);

            if (pos < size - 1)
            {
                self.inner_iter().updateKey(std::get<0>(sums)[1]);
            }

            self.outer_iter().addSize(-1);

            return true;
        }
        else {
            return false;
        }
    }

    void dump() const
    {
        self().outer_iter().dump();
        self().inner_iter().dump();
    }

    bool operator++(int)
    {
        auto& self = this->self();

        if (!self.outer_iter().isEnd())
        {
            CtrSizeT pos = self.pos();
            CtrSizeT size = self.size();

            if (self.outer_iter()++)
            {
                self.inner_iter().skipFw(size - pos);
                self.inner_iter().resetKeyPrefix();
                return true;
            }
            else {
                return false;
            }
        }
        else {
            return false;
        }
    }

    bool operator--(int)
    {
        auto& self = this->self();

        if (!self.outer_iter().isBegin())
        {
            CtrSizeT pos = self.pos();

            if (self.outer_iter()--)
            {
                CtrSizeT size = self.size();

                self.inner_iter().skipBw(size + pos);
                self.inner_iter().resetKeyPrefix();
                return true;
            }
            else {
                return false;
            }
        }
        else {
            return false;
        }
    }

    CtrSizeT skipFw(CtrSizeT delta)
    {
        auto& self = this->self();

        CtrSizeT size = self.size();
        CtrSizeT pos    = self.pos();

        CtrSizeT to_skip = (pos + delta < size) ? delta : size - pos;

        return self.inner_iter().skipFw(to_skip);
    }

    CtrSizeT skipBw(CtrSizeT delta)
    {
        auto& self = this->self();

        CtrSizeT pos    = self.pos();

        CtrSizeT to_skip = (pos - delta >= 0) ? delta : pos;

        return self.inner_iter().skipBw(to_skip);
    }

    bool isEof() const
    {
        auto& self = this->self();

        CtrSizeT size = self.size();
        CtrSizeT pos    = self.pos();

        return pos >= size;
    }

    bool isEnd() const
    {
        auto& self = this->self();
        return self.outer_iter().isEnd();
    }

    bool is_found_eq(Key key) const
    {
        auto& self = this->self();

        if (!self.isEnd()) {
            auto k0 = self.key();
            return k0 == key;
        }
        else {
            return false;
        }
    }


MEMORIA_ITERATOR_PART_END
}


#endif
