
// Copyright Victor Smirnov 2011-2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_CONTAINER_PAGES_HPP
#define	_MEMORIA_CORE_CONTAINER_PAGES_HPP

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

#pragma pack(1)




struct MEMORIA_API Page {

    virtual IDValue GetId() const                    = 0;
    virtual Int GetContainerHash() const             = 0;
    virtual Int GetPageTypeHash() const              = 0;
    virtual BigInt GetFlags() const                  = 0;
    virtual const void* Ptr() const                  = 0;
    virtual void* Ptr()                  		 	 = 0;
    virtual void SetPtr(void* ptr)              	 = 0;
    virtual bool IsNull() const						 = 0;

    virtual Int Size() const                         = 0;
    virtual Int GetByte(Int idx) const               = 0;
    virtual void SetByte(Int idx, Int value)    	 = 0;
};



template <typename PageType, Int PageSize>
class PageWrapper: public Page {
    PageType *page_;
public:
    PageWrapper(PageType* page): page_(page) {}
    PageWrapper(): page_(NULL) {}

    virtual bool IsNull() const {
    	return page_ == NULL;
    }

    virtual IDValue GetId() const
    {
    	if (page_ != NULL)
    	{
    		return IDValue(&page_->id());
    	}
    	else {
    		throw NullPointerException(MEMORIA_SOURCE, "Page data is not set");
    	}
    }

    virtual Int GetContainerHash() const
    {
    	if (page_ != NULL)
    	{
    		return page_->model_hash();
    	}
    	else {
    		throw NullPointerException(MEMORIA_SOURCE, "Page data is not set");
    	}
    }

    virtual Int GetPageTypeHash() const
    {
    	if (page_ != NULL)
    	{
    		return page_->page_type_hash();
    	}
    	else {
    		throw NullPointerException(MEMORIA_SOURCE, "Page data is not set");
    	}
    }
    
    virtual BigInt GetFlags() const {
        return 0;
    }
    
    virtual void* Ptr() {
        return page_;
    }

    virtual const void* Ptr() const {
        return page_;
    }

    virtual void SetPtr(void* ptr)
    {
    	page_ = static_cast<PageType*>(ptr);
    }

    virtual Int Size() const {
        return PageSize;
    }

    virtual Int GetByte(Int idx) const
    {
    	if (page_ != NULL)
    	{
    		if (idx >= 0 && idx < PageSize) {
    			return ((UByte*)page_)[idx];
    		}
    		else {
    			throw BoundsException(MEMORIA_SOURCE, SBuf()<<"Invalid byte offset: "<<idx<<" max="<<PageSize);
    		}

    	}
    	else {
    		throw NullPointerException(MEMORIA_SOURCE, "Page data is not set");
    	}
    }

    virtual void SetByte(Int idx, Int value)
    {
    	if (page_ != NULL)
    	{
    		if (idx >= 0 && idx < PageSize) {
    			((UByte*)page_)[idx] = (UByte)value;
    		}
    		else {
    			throw BoundsException(MEMORIA_SOURCE, SBuf()<<"Invalid byte offset: "<<idx<<" max="<<PageSize);
    		}
    	}
    	else {
    		throw NullPointerException(MEMORIA_SOURCE, "Page data is not set");
    	}
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
class PageHelper: public PagePart<typename SelectByIndexTool<Idx, typename Types::List>::Result, PageHelper<Idx - 1, Types> > {

};

template <typename Types>
class PageHelper<-1, Types>: public Types::NodePageBase {

};


template <typename Types>
class PageStart: public PageHelper<ListSize<typename Types::List>::Value - 1, Types> {

};

#pragma pack()

}






#include <memoria/core/container/page_traits.hpp>

#endif	// _MEMORIA_CORE_CONTAINER_PAGES_HPP
