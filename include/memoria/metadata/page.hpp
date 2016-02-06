
// Copyright Victor Smirnov 2011-2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_VAPI_METADATA_PAGE_HPP
#define _MEMORIA_VAPI_METADATA_PAGE_HPP

#include <memoria/metadata/group.hpp>
#include <memoria/core/tools/dump.hpp>
#include <memoria/core/tools/uuid.hpp>

#include <tuple>

namespace memoria    {

template <typename T> class PageID;


namespace vapi       {

enum {BTREE = 1, ROOT = 2, LEAF = 4, BITMAP = 8};


struct IPageLayoutEventHandler {

    virtual void startPage(const char* name)                                        = 0;
    virtual void endPage()                                                          = 0;

    virtual void startGroup(const char* name, Int elements = -1)                    = 0;
    virtual void endGroup()                                                         = 0;

    virtual void Layout(const char* name, Int type, Int ptr, Int size, Int count)   = 0;

    virtual ~IPageLayoutEventHandler(){}
};

struct IPageDataEventHandler {

    enum {BYTE_ARRAY = 100, BITMAP};

    virtual ~IPageDataEventHandler() {}

    virtual void startPage(const char* name, const void* ptr)                                   = 0;
    virtual void endPage()                                                                      = 0;

    virtual void startLine(const char* name, Int size = -1)                                     = 0;
    virtual void endLine()                                                                      = 0;

    virtual void startGroupWithAddr(const char* name, const void* ptr)               			= 0;
    virtual void startGroup(const char* name, Int elements = -1)               					= 0;
    virtual void endGroup()                                                                     = 0;

    virtual void value(const char* name, const Byte* value, Int count = 1, Int kind = 0)        = 0;
    virtual void value(const char* name, const UByte* value, Int count = 1, Int kind = 0)       = 0;
    virtual void value(const char* name, const Short* value, Int count = 1, Int kind = 0)       = 0;
    virtual void value(const char* name, const UShort* value, Int count = 1, Int kind = 0)      = 0;
    virtual void value(const char* name, const Int* value, Int count = 1, Int kind = 0)         = 0;
    virtual void value(const char* name, const UInt* value, Int count = 1, Int kind = 0)        = 0;
    virtual void value(const char* name, const BigInt* value, Int count = 1, Int kind = 0)      = 0;
    virtual void value(const char* name, const UBigInt* value, Int count = 1, Int kind = 0)     = 0;
    virtual void value(const char* name, const IDValue* value, Int count = 1, Int kind = 0)     = 0;
    virtual void value(const char* name, const float* value, Int count = 1, Int kind = 0)     	= 0;
    virtual void value(const char* name, const double* value, Int count = 1, Int kind = 0)      = 0;
    virtual void value(const char* name, const UUID* value, Int count = 1, Int kind = 0)        = 0;

    virtual void symbols(const char* name, const UBigInt* value, Int count, Int bits_per_symbol)    = 0;
    virtual void symbols(const char* name, const UByte* value, Int count, Int bits_per_symbol)      = 0;

    virtual void startStruct() 																	= 0;
    virtual void endStruct() 																	= 0;
};

struct DataEventsParams {};
struct LayoutEventsParams {};

struct IPageOperations
{
    virtual Int serialize(const void* page, void* buf) const                                    = 0;
    virtual void deserialize(const void* buf, Int buf_size, void* page) const                   = 0;
    virtual Int getPageSize(const void *page) const                                             = 0;

    virtual void resize(const void* page, void* buffer, Int new_size) const                     = 0;

    virtual void generateDataEvents(
                    const void* page,
                    const DataEventsParams& params,
                    IPageDataEventHandler* handler) const                                       = 0;

    virtual void generateLayoutEvents(
                    const void* page,
                    const LayoutEventsParams& params,
                    IPageLayoutEventHandler* handler) const                                     = 0;

    virtual ~IPageOperations() {}
};


struct MEMORIA_API PageMetadata: public MetadataGroup
{
    PageMetadata(
            StringRef name,
            Int attributes,
            Int hash0,
            const IPageOperations* page_operations);

    virtual ~PageMetadata() throw () {
    	delete page_operations_;
    }

    virtual Int hash() const {
        return hash_;
    }


    virtual const IPageOperations* getPageOperations() const {
        return page_operations_;
    }

private:
    Int  hash_;

    const IPageOperations* page_operations_;
};


template <typename T>
struct ValueHelper {
    static void setup(IPageDataEventHandler* handler, const char* name, const T& value)
    {
        handler->value(name, &value);
    }

    static void setup(IPageDataEventHandler* handler, const char* name, const T* value, Int size, Int type)
    {
    	handler->value(name, value, size, type);
    }
};

template <typename T>
struct ValueHelper<PageID<T> > {
    typedef PageID<T>                                                   Type;

    static void setup(IPageDataEventHandler* handler, const char* name, const Type& value)
    {
        IDValue id(&value);
        handler->value(name, &id);
    }

    static void setup(IPageDataEventHandler* handler, const char* name, const Type* value, Int size, Int type)
    {
    	for (Int c = 0; c < size; c++)
    	{
    		IDValue id(value + c);
    		handler->value(name, &id);
    	}
    }
};

template <>
struct ValueHelper<EmptyValue> {
    typedef EmptyValue Type;

    static void setup(IPageDataEventHandler* handler, const char* name, const Type& value)
    {
        BigInt val = 0;
        handler->value(name, &val);
    }
};



namespace internal {

template <typename Tuple, Int Idx = std::tuple_size<Tuple>::value - 1>
struct TupleValueHelper {

    using CurrentType = typename std::tuple_element<Idx, Tuple>::type;

    static void setup(IPageDataEventHandler* handler, const Tuple& field)
    {
        ValueHelper<CurrentType>::setup(handler, std::get<Idx>(field));

        TupleValueHelper<Tuple, Idx - 1>::setup(handler, field);
    }
};



template <typename Tuple>
struct TupleValueHelper<Tuple, -1> {
    static void setup(IPageDataEventHandler* handler, const Tuple& field) {}
};

}


template <typename... Types>
struct ValueHelper<std::tuple<Types...>> {

    using Type = std::tuple<Types...>;

    static void setup(IPageDataEventHandler* handler, const Type& value)
    {
        handler->startLine("VALUE", std::tuple_size<Type>::value);

        internal::TupleValueHelper<Type>::setup(handler, value);

        handler->endLine();
    }
};




void Expand(std::ostream& os, Int level);

class TextPageDumper: public IPageDataEventHandler {
    std::ostream& out_;

    Int level_;
    Int cnt_;

    bool line_;

public:
    TextPageDumper(std::ostream& out): out_(out), level_(0), cnt_(0), line_(false) {}
    virtual ~TextPageDumper() {}

    virtual void startPage(const char* name, const void* ptr)
    {
        out_<<name<<" at "<<ptr<<endl;
        level_++;
    }

    virtual void endPage()
    {
        out_<<endl;
        level_--;
    }

    virtual void startGroupWithAddr(const char* name, const void* ptr) {
        cnt_ = 0;
        memoria::Expand(out_, level_++);

        out_<<name;

        out_<<" at "<<ptr;
        out_<<endl;
    }

    virtual void startGroup(const char* name, Int elements = -1)
    {
        cnt_ = 0;
        memoria::Expand(out_, level_++);

        out_<<name;

        if (elements >= 0)
        {
            out_<<": "<<elements;
        }

        out_<<endl;
    };

    virtual void endGroup()
    {
        level_--;
    }


    virtual void startLine(const char* name, Int size = -1)
    {
        dumpLineHeader(out_, level_, cnt_++, name);
        line_ = true;
    }

    virtual void endLine()
    {
        line_ = false;
        out_<<endl;
    }



    virtual void value(const char* name, const Byte* value, Int count = 1, Int kind = 0)
    {
        if (kind == BYTE_ARRAY)
        {
            ::memoria::dumpArray<Byte>(out_, count, [=](Int idx){return value[idx];});
        }
        else {
            OutNumber(name, value, count, kind);
        }
    }

    virtual void value(const char* name, const UByte* value, Int count = 1, Int kind = 0)
    {
        if (kind == BYTE_ARRAY)
        {
            ::memoria::dumpArray<UByte>(out_, count, [=](Int idx){return value[idx];});
        }
        else {
            OutNumber(name, value, count, kind);
        }
    }

    virtual void value(const char* name, const Short* value, Int count = 1, Int kind = 0)
    {
        if (kind == BYTE_ARRAY)
        {
            ::memoria::dumpArray<Short>(out_, count, [=](Int idx){return value[idx];});
        }
        else {
            OutNumber(name, value, count, kind);
        }
    }


    virtual void value(const char* name, const UShort* value, Int count = 1, Int kind = 0)
    {
        if (kind == BYTE_ARRAY)
        {
            ::memoria::dumpArray<UShort>(out_, count, [=](Int idx){return value[idx];});
        }
        else {
            OutNumber(name, value, count, kind);
        }
    }

    virtual void value(const char* name, const Int* value, Int count = 1, Int kind = 0)
    {
        if (kind == BYTE_ARRAY)
        {
            ::memoria::dumpArray<Int>(out_, count, [=](Int idx){return value[idx];});
        }
        else {
            OutNumber(name, value, count, kind);
        }
    }


    virtual void value(const char* name, const UInt* value, Int count = 1, Int kind = 0)
    {
        if (kind == BYTE_ARRAY)
        {
            ::memoria::dumpArray<UInt>(out_, count, [=](Int idx){return value[idx];});
        }
        else {
            OutNumber(name, value, count, kind);
        }
    }

    virtual void value(const char* name, const BigInt* value, Int count = 1, Int kind = 0)
    {
        if (kind == BYTE_ARRAY)
        {
            ::memoria::dumpArray<BigInt>(out_, count, [=](Int idx){return value[idx];});
        }
        else {
            OutNumber(name, value, count, kind);
        }
    }

    virtual void value(const char* name, const UBigInt* value, Int count = 1, Int kind = 0)
    {
        if (kind == BYTE_ARRAY)
        {
            ::memoria::dumpArray<UBigInt>(out_, count, [=](Int idx){return value[idx];});
        }
        else {
            OutNumber(name, value, count, kind);
        }
    }

    virtual void value(const char* name, const float* value, Int count = 1, Int kind = 0)
    {
        if (kind == BYTE_ARRAY)
        {
            ::memoria::dumpArray<float>(out_, count, [=](Int idx){return value[idx];});
        }
        else {
            OutNumber(name, value, count, kind);
        }
    }

    virtual void value(const char* name, const double* value, Int count = 1, Int kind = 0)
    {
        if (kind == BYTE_ARRAY)
        {
            ::memoria::dumpArray<double>(out_, count, [=](Int idx){return value[idx];});
        }
        else {
            OutNumber(name, value, count, kind);
        }
    }


    virtual void value(const char* name, const IDValue* value, Int count = 1, Int kind = 0)
    {
        if (!line_)
        {
            dumpFieldHeader(out_, level_, cnt_++, name);
        }
        else {
            out_<<"    "<<name<<" ";
        }

        for (Int c = 0; c < count; c++)
        {
            out_<<*value;

            if (c < count - 1)
            {
                out_<<", ";
            }
        }

        if (!line_)
        {
            out_<<endl;
        }
    }

    virtual void value(const char* name, const UUID* value, Int count = 1, Int kind = 0)
    {
    	if (kind == BYTE_ARRAY)
    	{
    		::memoria::dumpArray<UUID>(out_, count, [=](Int idx){return value[idx];});
    	}
    	else {
    		OutNumber(name, value, count, kind);
    	}
    }


    virtual void symbols(const char* name, const UBigInt* value, Int count, Int bits_per_symbol)
    {
        dumpSymbols(out_, value, count, bits_per_symbol);
    }

    virtual void symbols(const char* name, const UByte* value, Int count, Int bits_per_symbol)
    {
        dumpSymbols(out_, value, count, bits_per_symbol);
    }


    virtual void startStruct() {
    	out_<<std::endl;
    }

    virtual void endStruct() {}

private:




    void dumpFieldHeader(ostream &out, Int level, Int idx, StringRef name)
    {
        stringstream str;
        memoria::Expand(str, level);
        str<<"FIELD: ";
        str<<idx<<" "<<name;

        int size = str.str().size();
        memoria::Expand(str, 30 - size);
        out<<str.str();
    }

    void dumpLineHeader(ostream &out, Int level, Int idx, StringRef name)
    {
        stringstream str;
        memoria::Expand(str, level);
        str<<name<<": ";
        str<<idx<<" ";

        int size = str.str().size();
        memoria::Expand(str, 15 - size);
        out<<str.str();
    }



    template <typename T>
    void OutNumber(const char* name, const T* value, Int count, Int kind)
    {
        if (!line_)
        {
            dumpFieldHeader(out_, level_, cnt_++, name);
        }
        else {
            out_<<"    "<<name<<" ";
        }

        for (Int c = 0; c < count; c++)
        {
            out_.width(12);
            out_<<*(value + c);

            if (c < count - 1)
            {
                out_<<",";
            }

            out_<<" ";
        }

        if (!line_)
        {
            out_<<endl;
        }
    }
};





}}

#endif
