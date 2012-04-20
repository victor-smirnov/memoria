
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_DYNVECTOR_PAGES_DATA_PAGE_HPP
#define	_MEMORIA_PROTOTYPES_DYNVECTOR_PAGES_DATA_PAGE_HPP

#include <memoria/core/tools/reflection.hpp>


namespace memoria    {
namespace dynvector {

#pragma pack(1)

template <
        typename ComponentList,
        template <Int> class DataBlockTypeFactory,
        typename Base0
>
class DVDataPage: public PageBuilder<ComponentList, Base0>
{

public:

    typedef DVDataPage<
                ComponentList,
                DataBlockTypeFactory,
                Base0
    >                                                                           	Me;

    typedef PageBuilder<ComponentList, Base0>                      					Base;

    typedef typename Base0::Allocator                                         		Allocator;

public:
    
    typedef typename DataBlockTypeFactory<Allocator::PAGE_SIZE - sizeof(Base)>::Type PageData;

private:

    PageData data_;

    static PageMetadata *reflection_;

public:

    DVDataPage(): Base(), data_() {}

    void init() {
    	data_.init();
    	Base::init();
    }

    Short size() const {
    	return data_.size();
    }

    const PageData& data() const {
        return data_;
    }

    PageData& data() {
        return data_;
    }

    static Int hash() {
        return reflection()->Hash();
    }

    static PageMetadata *reflection()
    {
    	return reflection_;
    }

    void Reindex()
    {
        Base::Reindex();
        data_.reindex();
    }

    Int data_size() const
    {
        Me* me = NULL;
        //FIXME: strict alias ?????????
        //Use c++11 offsetof
        return ((Int)(BigInt)&me->data_) + data_.byte_size();
    }

    static Int get_max_size() {
        return PageData::max_size();
    }

    void GenerateDataEvents(IPageDataEventHandler* handler) const
    {
    	Base::GenerateDataEvents(handler);
    	data_.GenerateDataEvents(handler);
    }

    template <template <typename> class FieldFactory>
    void Serialize(SerializationData& buf) const
    {
    	Base::template Serialize<FieldFactory>(buf);

    	FieldFactory<PageData>::serialize(buf, data_);
    }

    template <template <typename> class FieldFactory>
    void Deserialize(DeserializationData& buf)
    {
    	Base::template Deserialize<FieldFactory>(buf);

    	FieldFactory<PageData>::deserialize(buf, data_);
    }


    class PageOperations: public IPageOperations
    {
    	virtual Int Serialize(const void* page, void* buf) const
    	{
    		const Me* me = T2T<const Me*>(page);

    		SerializationData data;
    		data.buf = T2T<char*>(buf);

    		me->template Serialize<FieldFactory>(data);

    		return data.total;
    	}

    	virtual void Deserialize(const void* buf, Int buf_size, void* page) const
    	{
    		Me* me = T2T<Me*>(page);

    		DeserializationData data;
    		data.buf = T2T<const char*>(buf);

    		me->template Deserialize<FieldFactory>(data);
    	}

    	virtual Int GetPageSize(const void *page) const	{
    		const Me* me = T2T<const Me*>(page);
    		return me->data_size();
    	}

    	virtual void GenerateDataEvents(const void* page, const DataEventsParams& params, IPageDataEventHandler* handler) const
    	{
    		const Me* me = T2T<const Me*>(page);
    		handler->StartPage("DATA_PAGE");
    		me->GenerateDataEvents(handler);
    		handler->StartPage("DATA_PAGE");
    	}

    	virtual void GenerateLayoutEvents(const void* page, const LayoutEventsParams& params, IPageLayoutEventHandler* handler) const
    	{
    		const Me* me = T2T<const Me*>(page);
    		me->GenerateLayoutEvents(handler);
    	}
    };

    static Int Init()
    {
        if (reflection_ == NULL)
        {
            MetadataList list;

            Int hash0 = 1234567;
            Int attrs = BITMAP;

            reflection_ = new PageMetadataImpl("DATA_PAGE", list, attrs, hash0, new PageOperations(), Allocator::PAGE_SIZE);
        }
        else {}

        return reflection_->Hash();
    }
};


template <
        typename ComponentList,
        template <Int> class DataBlockTypeFactory,
        typename BaseType
>
PageMetadata* DVDataPage<ComponentList, DataBlockTypeFactory, BaseType>::reflection_ = NULL;

#pragma pack()

}
}

#endif
