
// Copyright Victor Smirnov 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_CONTAINER_PAGES_HPP
#define _MEMORIA_CORE_CONTAINER_PAGES_HPP

#include <memoria/core/types/typelist.hpp>
#include <memoria/core/container/builder.hpp>

#include <memoria/core/tools/bitmap.hpp>
#include <memoria/core/tools/buffer.hpp>
#include <memoria/core/tools/reflection.hpp>
#include <memoria/core/types/typehash.hpp>
#include <memoria/core/tools/reflection.hpp>
#include <memoria/core/tools/id.hpp>

#include <memoria/core/exceptions/exceptions.hpp>

namespace memoria    {

using namespace memoria::vapi;



struct MEMORIA_API Page {

    virtual IDValue getId() const                    = 0;
    virtual Int getContainerHash() const             = 0;
    virtual Int getPageTypeHash() const              = 0;
    virtual BigInt getFlags() const                  = 0;
    virtual const void* Ptr() const                  = 0;
    virtual void* Ptr()                              = 0;
    virtual void setPtr(void* ptr)                   = 0;
    virtual bool isNull() const                      = 0;

    virtual Int size() const                         = 0;
    virtual Int getByte(Int idx) const               = 0;
    virtual void setByte(Int idx, Int value)         = 0;

    virtual ~Page() throw() {}
};



template <typename PageType>
class PageWrapper: public Page {
    PageType *page_;
public:
    PageWrapper(PageType* page): page_(page) {}
    PageWrapper(): page_(NULL) {}

    virtual ~PageWrapper() throw()  {}

    virtual bool isNull() const {
        return page_ == NULL;
    }

    virtual IDValue getId() const
    {
        if (page_ != NULL)
        {
            return IDValue(&page_->id());
        }
        else {
            throw NullPointerException(MEMORIA_SOURCE, "Page data is not set");
        }
    }

    virtual Int getContainerHash() const
    {
        if (page_ != NULL)
        {
            return page_->ctr_type_hash();
        }
        else {
            throw NullPointerException(MEMORIA_SOURCE, "Page data is not set");
        }
    }

    virtual Int getPageTypeHash() const
    {
        if (page_ != NULL)
        {
            return page_->page_type_hash();
        }
        else {
            throw NullPointerException(MEMORIA_SOURCE, "Page data is not set");
        }
    }
    
    virtual BigInt getFlags() const {
        return 0;
    }
    
    virtual void* Ptr() {
        return page_;
    }

    virtual const void* Ptr() const {
        return page_;
    }

    virtual void setPtr(void* ptr)
    {
        page_ = static_cast<PageType*>(ptr);
    }

    virtual Int size() const {
        return page_->page_size();
    }

    virtual Int getByte(Int idx) const
    {
        if (page_ != NULL)
        {
            if (idx >= 0 && idx < page_->page_size()) {
                return T2T<UByte*>(page_)[idx];
            }
            else {
                throw BoundsException(MEMORIA_SOURCE, SBuf()<<"Invalid byte offset: "<<idx<<" max="<<page_->page_size());
            }

        }
        else {
            throw NullPointerException(MEMORIA_SOURCE, "Page data is not set");
        }
    }

    virtual void setByte(Int idx, Int value)
    {
        if (page_ != NULL)
        {
            if (idx >= 0 && idx < page_->page_size())
            {
                T2T<UByte*>(page_)[idx] = (UByte)value;
            }
            else {
                throw BoundsException(MEMORIA_SOURCE, SBuf()<<"Invalid byte offset: "<<idx<<" max="<<page_->page_size());
            }
        }
        else {
            throw NullPointerException(MEMORIA_SOURCE, "Page data is not set");
        }
    }
};


template <typename PageType>
class PageWrapper<const PageType>: public Page {
    const PageType *page_;
public:
    PageWrapper(const PageType* page): page_(page) {}
    PageWrapper(): page_(NULL) {}

    virtual ~PageWrapper() throw()  {}

    virtual bool isNull() const {
        return page_ == NULL;
    }

    virtual IDValue getId() const
    {
        if (page_ != NULL)
        {
            return IDValue(&page_->id());
        }
        else {
            throw NullPointerException(MEMORIA_SOURCE, "Page data is not set");
        }
    }

    virtual Int getContainerHash() const
    {
        if (page_ != NULL)
        {
            return page_->ctr_type_hash();
        }
        else {
            throw NullPointerException(MEMORIA_SOURCE, "Page data is not set");
        }
    }

    virtual Int getPageTypeHash() const
    {
        if (page_ != NULL)
        {
            return page_->page_type_hash();
        }
        else {
            throw NullPointerException(MEMORIA_SOURCE, "Page data is not set");
        }
    }

    virtual BigInt getFlags() const {
        return 0;
    }

    virtual void* Ptr() {
        throw Exception(MA_SRC, "Page in not mutable");
    }

    virtual const void* Ptr() const {
        return page_;
    }

    virtual void setPtr(void* ptr)
    {
        throw Exception(MA_SRC, "Page in not mutable");
    }

    virtual Int size() const {
        return page_->page_size();
    }

    virtual Int getByte(Int idx) const
    {
        if (page_ != NULL)
        {
            if (idx >= 0 && idx < page_->page_size()) {
                return T2T<UByte*>(page_)[idx];
            }
            else {
                throw BoundsException(MEMORIA_SOURCE, SBuf()<<"Invalid byte offset: "<<idx<<" max="<<page_->page_size());
            }

        }
        else {
            throw NullPointerException(MEMORIA_SOURCE, "Page data is not set");
        }
    }

    virtual void setByte(Int idx, Int value)
    {
        throw Exception(MA_SRC, "Page in not mutable");
    }
};



template <typename Name, typename Base>
class PagePart;

template <typename Name>
class PagePartNotFound;


class EmptyPart{};




template <
        typename PartsList,
        typename Base
>
struct PageBuilder: public Builder<PartsList, PagePart, Base> {};


template <int Idx, typename Types>
class PageHelper: public PagePart<
                    SelectByIndex<Idx, typename Types::List>,
                    PageHelper<Idx - 1, Types>
                  >
{
};

template <typename Types>
class PageHelper<-1, Types>: public Types::NodePageBase {

};


template <typename Types>
class PageStart: public PageHelper<ListSize<typename Types::List>::Value - 1, Types> {

};

}






#include <memoria/core/container/page_traits.hpp>

#endif  // _MEMORIA_CORE_CONTAINER_PAGES_HPP
