
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_DYNVECTOR_PAGES_DATA_PAGE_HPP
#define _MEMORIA_PROTOTYPES_DYNVECTOR_PAGES_DATA_PAGE_HPP

#include <memoria/core/tools/reflection.hpp>


namespace memoria    {
namespace dynvector {

#pragma pack(1)

template <
        typename ComponentList,
        typename DataBlock,
        typename Base0
>
class DVDataPage: public PageBuilder<ComponentList, Base0>
{
    static const UInt VERSION = 1;

public:

    typedef DVDataPage<
                ComponentList,
                DataBlock,
                Base0
    >                                                                           MyType;

    typedef PageBuilder<ComponentList, Base0>                                   Base;
    
    typedef DataBlock                                                           PageData;


    typedef typename MergeLists <
                typename Base::FieldsList,
                ConstValue<UInt, VERSION>,
                typename PageData::FieldsList
    >::Result                                                                   FieldsList;

    static const UInt PAGE_HASH = md5::Md5Sum<typename TypeToValueList<FieldsList>::Type>::Result::Value32;

    typedef typename PageData::ElementType                                      ElementType;

private:

    PageData data_;

    static PageMetadata *page_metadata_;

public:

    DVDataPage(): Base(), data_() {}

    void init() {
        data_.init();
        Base::init();
    }

    Int size() const {
        return data_.size();
    }

    const PageData& data() const {
        return data_;
    }

    PageData& data() {
        return data_;
    }

    static Int hash() {
        return PAGE_HASH;
    }

    static PageMetadata *page_metadata()
    {
        return page_metadata_;
    }

    void Reindex()
    {
        Base::Reindex();
        data_.reindex();
    }

    Int data_size() const
    {
        return sizeof(MyType) + data_.data_size();
    }

    Int getCapacity() const {
        return getMaxCapacity() - data_.size();
    }

    Int getMaxCapacity() const {
        return (Base::page_size() - sizeof(MyType)) / sizeof(ElementType);
    }

    const ElementType* addr(Int idx) const {
        return data_.value_addr(idx);
    }

    ElementType* addr(Int idx) {
        return data_.value_addr(idx);
    }

    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        Base::generateDataEvents(handler);
        data_.generateDataEvents(handler);
    }

    template <template <typename> class FieldFactory>
    void serialize(SerializationData& buf) const
    {
        Base::template serialize<FieldFactory>(buf);

        FieldFactory<PageData>::serialize(buf, data_);
    }

    template <template <typename> class FieldFactory>
    void deserialize(DeserializationData& buf)
    {
        Base::template deserialize<FieldFactory>(buf);

        FieldFactory<PageData>::deserialize(buf, data_);
    }


    class PageOperations: public IPageOperations
    {
        virtual Int serialize(const void* page, void* buf) const
        {
            const MyType* me = T2T<const MyType*>(page);

            SerializationData data;
            data.buf = T2T<char*>(buf);

            me->template serialize<FieldFactory>(data);

            return data.total;
        }

        virtual void deserialize(const void* buf, Int buf_size, void* page) const
        {
            MyType* me = T2T<MyType*>(page);

            DeserializationData data;
            data.buf = T2T<const char*>(buf);

            me->template deserialize<FieldFactory>(data);
        }

        virtual Int getPageSize(const void *page) const {
            const MyType* me = T2T<const MyType*>(page);
            return me->data_size();
        }

        virtual void resize(const void* page, void* buffer, Int new_size) const
        {
            const MyType* me = T2T<const MyType*>(page);
            MyType* tgt = T2T<MyType*>(buffer);

            tgt->copyFrom(me);
            me->data().copyTo(&tgt->data());
        }

        virtual void generateDataEvents(
                        const void* page,
                        const DataEventsParams& params,
                        IPageDataEventHandler* handler
                     ) const
        {
            const MyType* me = T2T<const MyType*>(page);
            handler->startPage("DATA_PAGE");
            me->generateDataEvents(handler);
            handler->startPage("DATA_PAGE");
        }

        virtual void generateLayoutEvents(
                        const void* page,
                        const LayoutEventsParams& params,
                        IPageLayoutEventHandler* handler
                     ) const
        {
            const MyType* me = T2T<const MyType*>(page);
            me->generateLayoutEvents(handler);
        }
    };

    static Int initMetadata()
    {
        if (page_metadata_ == NULL)
        {
            Int attrs = BITMAP;

            page_metadata_ = new PageMetadata("DATA_PAGE", attrs, PAGE_HASH, new PageOperations());
        }
        else {}

        return page_metadata_->hash();
    }
};


template <
        typename ComponentList,
        typename DataBlock,
        typename BaseType
>
PageMetadata* DVDataPage<ComponentList, DataBlock, BaseType>::page_metadata_ = NULL;

#pragma pack()

}
}

#endif
