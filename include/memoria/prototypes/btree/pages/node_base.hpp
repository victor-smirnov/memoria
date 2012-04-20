
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTREE_PAGES_NODE_BASE_HPP
#define	_MEMORIA_PROTOTYPES_BTREE_PAGES_NODE_BASE_HPP

#include <memoria/core/tools/reflection.hpp>
#include <memoria/core/container/page.hpp>
#include <memoria/core/container/logs.hpp>


namespace memoria    {
namespace btree     {

using memoria::BitBuffer;

#pragma pack(1)

template <typename Allocator_>
class TreePage: public Allocator_::Page {

    Int root_;
    Int leaf_;
    Int bitmap_;

    Int size_;

public:

    enum {
        LEAF          = 0,
        ROOT          = 1,
        BITMAP        = 2
    }                                       FLAGS;

    typedef Allocator_                        Allocator;

    typedef TreePage<Allocator>               Me;
    typedef typename Allocator::Page          Base;
    typedef typename Base::ID                 ID;

    TreePage(): Base() {}

    inline bool is_root() const {
        return root_;
    }

    void set_root(bool root) {
        root_ = root;
    }

    inline bool is_leaf() const {
        return leaf_;
    }

    void set_leaf(bool leaf) {
        leaf_ = leaf;
    }

    inline bool is_bitmap() const {
        return bitmap_;
    }

    void set_bitmap(bool bitmap) {
        bitmap_ = bitmap;
    }

    Int size() const
    {
    	return size_;
    }

    Int children_count() const
    {
    	return size_;
    }

protected:
    Int& map_size()
    {
    	return size_;
    }
public:

    void GenerateDataEvents(IPageDataEventHandler* handler) const
    {
    	Base::GenerateDataEvents(handler);

    	handler->Value("ROOT", &root_);
    	handler->Value("LEAF", &leaf_);
    	handler->Value("BITMAP", &bitmap_);
    	handler->Value("SIZE", &size_);
    }

    template <template <typename> class FieldFactory>
    void Serialize(SerializationData& buf) const
    {
    	Base::template Serialize<FieldFactory>(buf);

    	FieldFactory<Int>::serialize(buf, root_);
    	FieldFactory<Int>::serialize(buf, leaf_);
    	FieldFactory<Int>::serialize(buf, bitmap_);

    	FieldFactory<Int>::serialize(buf, size_);
    }

    template <template <typename> class FieldFactory>
    void Deserialize(DeserializationData& buf)
    {
    	Base::template Deserialize<FieldFactory>(buf);

    	FieldFactory<Int>::deserialize(buf, root_);
    	FieldFactory<Int>::deserialize(buf, leaf_);
    	FieldFactory<Int>::deserialize(buf, bitmap_);

    	FieldFactory<Int>::deserialize(buf, size_);
    }


    template <typename PageType>
    void CopyFrom(const PageType* page)
    {
        Base::CopyFrom(page);

        this->set_root(page->is_root());
        this->set_leaf(page->is_leaf());
        this->set_bitmap(page->is_bitmap());
    }
};


template <
		typename BaseType0
>
class NodePageBase: public BaseType0
{
    Short           level_;

public:
    typedef BaseType0                                                            Base;
    typedef BaseType0                                                            BasePageType;

    NodePageBase(): BaseType0(), level_(0)
    {
        init();
    }

    void init()
    {
    	Base::init();
    }

    const Short &level() const
    {
        return level_;
    }

    Short &level()
    {
        return level_;
    }

    void GenerateDataEvents(IPageDataEventHandler* handler) const
    {
    	Base::GenerateDataEvents(handler);
    	handler->Value("LEVEL", &level_);
    }


    template <template <typename> class FieldFactory>
    void Serialize(SerializationData& buf) const
    {
    	Base::template Serialize<FieldFactory>(buf);

    	FieldFactory<Short>::serialize(buf, level_);
    }

    template <template <typename> class FieldFactory>
    void Deserialize(DeserializationData& buf)
    {
    	Base::template Deserialize<FieldFactory>(buf);

    	FieldFactory<Short>::deserialize(buf, level_);
    }


    template <typename PageType>
    void CopyFrom(PageType* page)
    {
        Base::CopyFrom(page);

        this->level() = page->level();
    }
};

#pragma pack()

}
}





#endif
