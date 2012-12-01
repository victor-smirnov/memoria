
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_CONTAINER_DISPATCHER_HPP
#define _MEMORIA_CORE_CONTAINER_DISPATCHER_HPP

#include <memoria/core/container/profile.hpp>
#include <memoria/core/container/builder.hpp>
#include <memoria/core/container/names.hpp>


#include <memoria/core/types/typelist.hpp>



namespace memoria    {


template <typename Profile, typename PageType, typename List>
class PageDispatcher {

    typedef typename PageType::ID                                               PageId;

    typedef typename List::Head                                                 ContainerType;
    typedef typename ContainerType::Types::Pages::NodeDispatcher                NodeDispatcher;
    typedef typename ContainerType::Types::NodeBase                             NodeBase;

public:
    void dispatch(PageType *page) {
        if (page->model_hash() == ContainerType::hash()) {
            NodeDispatcher::dispatch(static_cast<NodeBase*>(page));
        }
        else {
            PageDispatcher<Profile, PageType, typename ListTail<List>::Type>::dispatch(page);
        }
    }
};

template <typename Profile, typename PageType>
class PageDispatcher<Profile, PageType, TypeList<> > {

public:
    void dispatch(PageType *page) {
        throw DispatchException(MEMORIA_SOURCE, SBuf()<<"Invalid model hash code"<<page->model_hash());
    }
};


template <
        typename Profile,
        typename Allocator,
        typename List1,
        typename List2 = List1,
        typename List3 = List1,
        typename List4 = List1
>
class ContainerDispatcher {

    typedef typename List1::Head::First Name1;
    typedef typename List1::Head::Second Head1;
    typedef typename List1::Tail Tail1;

    typedef typename Allocator::Page                                            Page;
    typedef typename Allocator::Page::ID                                        ID;

public:

    template <typename Handler>
    static void dispatch(Allocator *allocator, Page* root, Handler &functor) {
        if (root->model_hash() == Head1::hash())
        {
            Head1 model(*allocator, root->id());
            functor(*allocator, model);
        }
        else {
            ContainerDispatcher<Profile, Allocator, Tail1>::dispatch(allocator, root, functor);
        }
    }

    static void buildMetadataList(MetadataList &list) {
        Head1::initMetadata();
        Metadata* metadata = Head1::reflection();
        list.push_back(metadata);
        ContainerDispatcher<Profile, Allocator, Tail1>::buildMetadataList(list);
    }

    static void destroyMetadata() {
        ContainerDispatcher<Profile, Allocator, Tail1>::destroyMetadata();
    }

    static void printContainerHashes() {
        cout<<TypeNameFactory<Name1>::name()<<" "<<Head1::hash()<<endl;
        ContainerDispatcher<Profile, Allocator, Tail1>::printContainerHashes();
    }
};


template <
        typename Profile,
        typename Allocator,
        typename List2,
        typename List3,
        typename List4
>
class ContainerDispatcher<Profile, Allocator, NullType, List2, List3, List4> {

    typedef typename Allocator::Page                                            Page;
    typedef typename Allocator::Page::ID                                        ID;

public:
    template <typename Handler>
    static void dispatch(Allocator *allocator, Page* root, Handler &functor) {}

    static void buildMetadataList(MetadataList &list) {}
    static void destroyMetadata() {}
    static void printContainerHashes() {}
};



template <
        typename Profile,
        typename Allocator,
        typename List1,
        typename List3,
        typename List4
>
class ContainerDispatcher<Profile, Allocator, List1, NullType, List3, List4> {

    typedef typename Allocator::Page                                            Page;
    typedef typename Allocator::Page::ID                                        ID;

public:
    template <typename Handler>
    static void dispatch(Allocator *allocator, Page *root, Handler &functor) {}

    static void buildMetadataList(MetadataList &list) {}
    static void destroyMetadata() {}
    static void printContainerHashes() {}
};



template <
        typename Profile,
        typename Allocator,
        typename List1,
        typename List2,
        typename List4
>
class ContainerDispatcher<Profile, Allocator, List1, List2, NullType, List4> {

    typedef typename Allocator::Page                                            Page;
    typedef typename Allocator::Page::ID                                        ID;

public:
    template <typename Handler>
    static void dispatch(Allocator *allocator, Page *root, Handler &functor) {}

    static void buildMetadataList(MetadataList &list) {}
    static void destroyMetadata() {}
    static void printContainerHashes() {}
};


template <
        typename Profile,
        typename Allocator,
        typename List1,
        typename List2,
        typename List3
>
class ContainerDispatcher<Profile, Allocator, List1, List2, List3, NullType> {

    typedef typename Allocator::Page                                            Page;
    typedef typename Allocator::Page::ID                                        ID;

public:
    template <typename Handler>
    static void dispatch(Allocator *allocator, Page *root, Handler &functor) {}

    static void buildMetadataList(MetadataList &list) {}
    static void destroyMetadata() {}
    static void printContainerHashes() {}
};

template <
        typename Profile,
        typename Allocator
>
class ContainerDispatcher<Profile, Allocator, NullType, NullType, NullType, NullType> {

    typedef typename Allocator::Page                                            Page;
    typedef typename Allocator::Page::ID                                        ID;

public:
    template <typename Handler>
    static void dispatch(Allocator *allocator, Page *root, Handler &functor) {}

    static void buildMetadataList(MetadataList &list) {}

    static void destroyMetadata() {}
    static void printContainerHashes() {}
};



template <typename List> class PageInitDispatcher;

template <typename Head, typename ... Tail>
class PageInitDispatcher<TypeList<Head, Tail...> > {
public:
    static void buildMetadataList(MetadataList &list) {
        Head::initMetadata();
        list.push_back(Head::reflection());
        PageInitDispatcher<TypeList<Tail...>>::buildMetadataList(list);
    }

    static void destroyMetadata() {
        PageInitDispatcher<TypeList<Tail...>>::destroyMetadata();
    }
};

template <>
class PageInitDispatcher<TypeList<>> {
public:
    static void buildMetadataList(MetadataList &list) {}

    static void destroyMetadata() {}
};


}



#endif  // _MEMORIA_CORE_CONTAINER_DISPATCHER_HPP
