
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BTREE_PAGES_NODE_BASE_HPP
#define	_MEMORIA_PROTOTYPES_BTREE_PAGES_NODE_BASE_HPP

#include <memoria/core/tools/reflection.hpp>
#include <memoria/core/container/page.hpp>
#include <memoria/vapi/models/logs.hpp>


namespace memoria    {
namespace btree     {

using memoria::BitBuffer;

#pragma pack(1)

template <typename Allocator_>
class TreePage: public Allocator_::Page {
    
//    typedef BitBuffer<32>                   TreeFlagsType;
//
//    TreeFlagsType                           tree_flags_;

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

    template <template <typename> class FieldFactory>
    void BuildFieldsList(MetadataList &list, Long &abi_ptr) const {
        Base::template BuildFieldsList<FieldFactory>(list, abi_ptr);

//        abi_ptr += ValueTraits<TreeFlagsType>::Size;

        FieldFactory<Int>::create(list, root_, 		"ROOT", abi_ptr);
        FieldFactory<Int>::create(list, leaf_, 		"LEAF", abi_ptr);
        FieldFactory<Int>::create(list, bitmap_, 	"BITMAP", abi_ptr);

        FieldFactory<Int>::create(list, size_, "SIZE", abi_ptr);
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

template <typename CountType>
class BTreeCountersBase {

    CountType page_count_;
    CountType key_count_;

public:
    BTreeCountersBase(): page_count_(0), key_count_(0) {}

    BTreeCountersBase(CountType pc, CountType kc): page_count_(pc), key_count_(kc) {}

    const CountType &page_count() const
    {
        return page_count_;
    }

    CountType &page_count()
    {
        return page_count_;
    }

    const CountType &key_count() const
    {
        return key_count_;
    }

    CountType &key_count()
    {
        return key_count_;
    }

    BTreeCountersBase operator-() const {
        return BTreeCountersBase(-page_count(), -key_count());
    }

    BTreeCountersBase operator-(const BTreeCountersBase &other) const {
        return BTreeCountersBase(page_count() - other.page_count(), key_count() - other.key_count());
    }

    void operator-=(const BTreeCountersBase &other) {
        page_count() -= other.page_count();
        key_count() -= other.key_count();
    }

    BTreeCountersBase operator+(const BTreeCountersBase &other) const {
        return BTreeCountersBase(page_count() + other.page_count(), key_count() + other.key_count());
    }

    void operator+=(const BTreeCountersBase &other) {
        page_count() += other.page_count();
        key_count() += other.key_count();
    }

    bool operator==(const BTreeCountersBase &other) {
        return  page_count() == other.page_count() &&
                key_count() == other.key_count();
    }

    bool operator!=(const BTreeCountersBase &other) {
        return !operator==(other);
    }

    MetadataList GetFields(Long &abi_ptr) const {
        MetadataList list;
        FieldFactory<CountType>::create(list, page_count(), "PAGE_COUNT", abi_ptr);
        FieldFactory<CountType>::create(list, key_count(), "KEY_COUNT", abi_ptr);
        return list;
    }
};

template <typename T>
memoria::vapi::LogHandler* LogIt(memoria::vapi::LogHandler* log, const BTreeCountersBase<T>& value) {
    log->log("Counters[");
    log->log("page_count=");
    log->log(value.page_count());
    log->log("; key_count=");
    log->log(value.key_count());
    log->log("] ");
    return log;
}


template <
		typename CountersType,
        typename BaseType0
>
class NodePageBase: public BaseType0
{
    Short           level_;
    //CountersType    counters_;

public:
    typedef BaseType0                                                            Base;
    typedef BaseType0                                                            BasePageType;
    typedef CountersType                                                        Counters;

    NodePageBase(): BaseType0(), level_(0)
    {
        init();
    }

    void init() {
    	Base::init();
//    	counters().page_count() = 1;
    }

//    const Counters &counters() const
//    {
//        return counters_;
//    }
//
//    Counters &counters()
//    {
//        return counters_;
//    }

    const Short &level() const
    {
        return level_;
    }

    Short &level()
    {
        return level_;
    }

    template <template <typename> class FieldFactory>
    void BuildFieldsList(MetadataList &list, Long &abi_ptr) const {
        Base::template BuildFieldsList<FieldFactory>(list, abi_ptr);

        FieldFactory<Short>::create(list, level(), "LEVEL", abi_ptr);
//        FieldFactory<Counters>::create(list, counters(), "COUNTERS", abi_ptr);
    }

    template <typename PageType>
    void CopyFrom(PageType* page)
    {
        Base::CopyFrom(page);

        this->level()       = page->level();
//        this->counters()    = page->counters();
    }
};

#pragma pack()

}




using namespace memoria::btree;

template <typename Profile, typename Name>
class BTreeCountersTypeFactory<Profile, BTreeCountersFactory<Name> > {
public:
    typedef BTreeCountersBase<BigInt>                                           Type;
};


}





#endif
