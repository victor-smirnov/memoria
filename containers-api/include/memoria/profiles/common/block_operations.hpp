
// Copyright 2011-2025 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#pragma once

#include <memoria/core/tools/id.hpp>
#include <memoria/core/tools/dump.hpp>
#include <memoria/core/tools/uuid.hpp>
#include <memoria/core/tools/uid_64.hpp>
#include <memoria/core/tools/uid_256.hpp>
#include <memoria/core/tools/result.hpp>


#include <memoria/core/integer/integer.hpp>

#include <memoria/core/strings/strings.hpp>
#include <memoria/core/strings/format.hpp>
#include <memoria/core/bytes/bytes.hpp>
#include <memoria/core/memory/memory.hpp>

#include <memoria/profiles/common/common.hpp>

#include <tuple>
#include <functional>
#include <sstream>

namespace memoria {

template <typename T> class BlockID;

struct BlockDataValueProvider {
    virtual ~BlockDataValueProvider() noexcept {}

    virtual size_t size() const = 0;
    virtual bool isArray() const = 0;
    virtual U8String value(size_t idx) const = 0;
};

struct IBlockDataEventHandler {

    enum {BYTE_ARRAY = 100, BITMAP};

    virtual ~IBlockDataEventHandler() noexcept {}

    virtual void startBlock(const char* name, const void* ptr)                                  = 0;
    virtual void endBlock()                                                                     = 0;

    virtual void startLine(const char* name)                                                    = 0;
    virtual void endLine()                                                                      = 0;

    virtual void startGroupWithAddr(const char* name, const void* ptr)                          = 0;
    virtual void startGroup(const char* name, size_t elements = SizeTMax)                       = 0;
    virtual void endGroup()                                                                     = 0;

    virtual void value(const char* name, const int8_t* value, size_t count = 1, size_t kind = 0)      = 0;
    virtual void value(const char* name, const uint8_t* value, size_t count = 1, size_t kind = 0)     = 0;
    virtual void value(const char* name, const int16_t* value, size_t count = 1, size_t kind = 0)     = 0;
    virtual void value(const char* name, const uint16_t* value, size_t count = 1, size_t kind = 0)    = 0;
    virtual void value(const char* name, const int32_t* value, size_t count = 1, size_t kind = 0)     = 0;
    virtual void value(const char* name, const uint32_t* value, size_t count = 1, size_t kind = 0)    = 0;
    virtual void value(const char* name, const int64_t* value, size_t count = 1, size_t kind = 0)     = 0;
    virtual void value(const char* name, const uint64_t* value, size_t count = 1, size_t kind = 0)    = 0;
#ifdef MMA_HAS_INT128
    virtual void value(const char* name, const Int128T* value, size_t count = 1, size_t kind = 0)     = 0;
    virtual void value(const char* name, const UInt128T* value, size_t count = 1, size_t kind = 0)    = 0;
#endif
    virtual void value(const char* name, const IDValue* value, size_t count = 1, size_t kind = 0)     = 0;
    virtual void value(const char* name, const float* value, size_t count = 1, size_t kind = 0)       = 0;
    virtual void value(const char* name, const double* value, size_t count = 1, size_t kind = 0)      = 0;
    virtual void value(const char* name, const UUID* value, size_t count = 1, size_t kind = 0)        = 0;
    virtual void value(const char* name, const UID64* value, size_t count = 1, size_t kind = 0)       = 0;
    virtual void value(const char* name, const UID256* value, size_t count = 1, size_t kind = 0)      = 0;
    virtual void value(const char* name, const UAcc64T* value, size_t count = 1, size_t kind = 0)     = 0;
    virtual void value(const char* name, const UAcc128T* value, size_t count = 1, size_t kind = 0)    = 0;
    virtual void value(const char* name, const UAcc192T* value, size_t count = 1, size_t kind = 0)    = 0;
    virtual void value(const char* name, const UAcc256T* value, size_t count = 1, size_t kind = 0)    = 0;
    virtual void value(const char* name, const CowBlockID<UID64>* value, size_t count = 1, size_t kind = 0)  = 0;
    virtual void value(const char* name, const CowBlockID<UID256>* value, size_t count = 1, size_t kind = 0) = 0;
    virtual void value(const char* name, const CowBlockID<UUID>* value, size_t count = 1, size_t kind = 0)   = 0;

    virtual void symbols(const char* name, const uint64_t* value, size_t count, size_t bits_per_symbol)    = 0;
    virtual void symbols(const char* name, const uint8_t* value, size_t count, size_t bits_per_symbol)     = 0;

    virtual void value(const char* name, const BlockDataValueProvider& value)                    = 0;

    virtual void as_uint8_array(const char* name, size_t count, std::function<uint8_t(size_t)> fn) {
        MEMORIA_MAKE_GENERIC_ERROR("IBlockDataEventHandler::as_uint8_array is not implemented").do_throw();
    }

    virtual void startStruct()                                                                  = 0;
    virtual void endStruct()                                                                    = 0;
};

struct DataEventsParams {};
struct LayoutEventsParams {};



template <typename Profile>
struct IBlockOperationsBase {

    virtual ~IBlockOperationsBase() noexcept {}

    using BlockType = ProfileBlockType<Profile>;
    using BlockID   = ProfileBlockID<Profile>;

    struct IDValueResolver {
        virtual BlockID resolve_id(const BlockID& stored_block_id) const = 0;
    };

    virtual size_t serialize(
            const BlockType* block,
            void* buf,
            const IDValueResolver* resolver = nullptr
    ) const = 0;

    virtual void deserialize(const void* buf, size_t buf_size, BlockType* block) const = 0;

    virtual void resize(const BlockType* block, void* buffer, size_t new_size) const = 0;

    virtual uint64_t block_type_hash() const = 0;

    virtual void for_each_child(
            const BlockType* block,
            std::function<void (const BlockID&)> callback
    ) const = 0;
};

template <typename Profile>
struct IBlockOperations: IBlockOperationsBase<Profile> {

};





template <typename Profile>
using BlockOperationsPtr = std::shared_ptr<IBlockOperations<Profile>>;


template <typename T>
struct ValueHelper {
    static void setup(IBlockDataEventHandler* handler, const char* name, const T& value)
    {
        handler->value(name, &value);
    }

    static void setup(IBlockDataEventHandler* handler, const char* name, const T* value, size_t size, size_t type)
    {
        handler->value(name, value, size, type);
    }
};

template <typename T>
struct ValueHelper<BlockID<T> > {
    using Type = BlockID<T>;

    static void setup(IBlockDataEventHandler* handler, const char* name, const Type& value)
    {
        IDValue id(&value);
        handler->value(name, &id);
    }

    static void setup(IBlockDataEventHandler* handler, const char* name, const Type* value, size_t size, size_t type)
    {
        for (size_t c = 0; c < size; c++)
        {
            IDValue id(value + c);
            handler->value(name, &id);
        }
    }
};


namespace internal {

template <typename Tuple, int32_t Idx = std::tuple_size<Tuple>::value - 1>
struct TupleValueHelper {

    using CurrentType = typename std::tuple_element<Idx, Tuple>::type;

    static void setup(IBlockDataEventHandler* handler, const Tuple& field)
    {
        ValueHelper<CurrentType>::setup(handler, std::get<Idx>(field));
        TupleValueHelper<Tuple, Idx - 1>::setup(handler, field);
    }
};



template <typename Tuple>
struct TupleValueHelper<Tuple, -1> {
    static void setup(IBlockDataEventHandler* handler, const Tuple& field) {}
};

}


template <typename... Types>
struct ValueHelper<std::tuple<Types...>> {

    using Type = std::tuple<Types...>;

    static void setup(IBlockDataEventHandler* handler, const Type& value)
    {
        handler->startLine("VALUE", std::tuple_size<Type>::value);

        internal::TupleValueHelper<Type>::setup(handler, value);

        handler->endLine();
    }
};





template <typename Fn>
class BlockValueFnProviderT: public BlockDataValueProvider {
    Fn fn_;
    size_t size_;
    bool array_;
public:
    BlockValueFnProviderT(bool array, size_t size, Fn fn): fn_(fn), size_(size), array_(array) {}

    virtual size_t size() const {return size_;}
    virtual bool isArray() const {return array_;}
    virtual U8String value(size_t idx) const
    {
        if (idx >= 0 && idx < size_)
        {
            return toString(fn_(idx));
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Invalid index access in BlockValueProviderT: idx = {}, size = {}", idx, size_).do_throw();
        }
    }
};


template <typename T>
class BlockValueProviderT: public BlockDataValueProvider {

    const T& value_;

public:
    BlockValueProviderT(const T& value): value_(value) {}

    virtual size_t size() const {return 1;}
    virtual bool isArray() const {return false;}
    virtual U8String value(size_t idx) const
    {
        if (idx == 0)
        {
            return toString(value_);
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Invalid index access in BlockValueProviderT: idx = {}, size = 1", idx).do_throw();
        }
    }
};


template <>
class BlockValueProviderT<U8String>: public BlockDataValueProvider {

    const U8String& value_;

public:
    BlockValueProviderT(const U8String& value): value_(value) {}

    virtual size_t size() const {return 1;}
    virtual bool isArray() const {return false;}
    virtual U8String value(size_t idx) const
    {
        if (idx == 0)
        {
            return value_;
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Invalid index access in BlockValueProviderT: idx = {}, size = 1", idx).do_throw();
        }
    }
};


struct BlockValueProviderFactory {
    template <typename Fn>
    static auto provider(bool is_array, size_t size, Fn fn) {
        return BlockValueFnProviderT<Fn>(is_array, size, fn);
    }

    template <typename Fn>
    static auto provider(size_t size, Fn fn) {
        return BlockValueFnProviderT<Fn>(false, size, fn);
    }

    template <typename V>
    static auto provider(V&& v) {
        return BlockValueProviderT<V>(v);
    }
};



void Expand(std::ostream& os, size_t level);

void dumpPageDataValueProviderAsArray(std::ostream& out, const BlockDataValueProvider& provider);

class TextBlockDumper: public IBlockDataEventHandler {
    std::ostream& out_;

    size_t level_;
    size_t cnt_;

    bool line_;

public:
    TextBlockDumper(std::ostream& out): out_(out), level_(0), cnt_(0), line_(false) {}
    virtual ~TextBlockDumper() {}

    virtual void startBlock(const char* name, const void* ptr)
    {
        out_ << name << " at " << ptr << std::endl;
        level_++;
    }

    virtual void endBlock()
    {
        out_ << std::endl;
        level_--;
    }

    virtual void startGroupWithAddr(const char* name, const void* ptr) {
        cnt_ = 0;
        Expand(out_, level_++);

        out_ << name;

        out_ << " at " << ptr;
        out_ << std::endl;
    }

    virtual void startGroup(const char* name, size_t elements = SizeTMax)
    {
        cnt_ = 0;
        Expand(out_, level_++);

        out_ << name;

        if (elements < SizeTMax)
        {
            out_ << ": " << elements;
        }

        out_ << std::endl;
    }

    virtual void endGroup()
    {
        level_--;
    }


    virtual void startLine(const char* name)
    {
        dumpLineHeader(out_, level_, cnt_++, name);
        line_ = true;
    }

    virtual void endLine()
    {
        line_ = false;
        out_ << std::endl;
    }



    virtual void value(const char* name, const int8_t* value, size_t count = 1, size_t kind = 0)
    {
        if (kind == BYTE_ARRAY)
        {
            dumpArray<int8_t>(out_, count, [=](size_t idx){return value[idx];});
        }
        else {
            OutNumber(name, value, count, kind);
        }
    }

    virtual void value(const char* name, const uint8_t* value, size_t count = 1, size_t kind = 0)
    {
        if (kind == BYTE_ARRAY)
        {
            dumpArray<uint8_t>(out_, count, [=](size_t idx){return value[idx];});
        }
        else {
            OutNumber(name, value, count, kind);
        }
    }

    virtual void value(const char* name, const int16_t* value, size_t count = 1, size_t kind = 0)
    {
        if (kind == BYTE_ARRAY)
        {
            dumpArray<int16_t>(out_, count, [=](size_t idx){return value[idx];});
        }
        else {
            OutNumber(name, value, count, kind);
        }
    }


    virtual void value(const char* name, const uint16_t* value, size_t count = 1, size_t kind = 0)
    {
        if (kind == BYTE_ARRAY)
        {
            dumpArray<uint16_t>(out_, count, [=](size_t idx){return value[idx];});
        }
        else {
            OutNumber(name, value, count, kind);
        }
    }

    virtual void value(const char* name, const int32_t* value, size_t count = 1, size_t kind = 0)
    {
        if (kind == BYTE_ARRAY)
        {
            dumpArray<size_t>(out_, count, [=](size_t idx){return value[idx];});
        }
        else {
            OutNumber(name, value, count, kind);
        }
    }


    virtual void value(const char* name, const uint32_t* value, size_t count = 1, size_t kind = 0)
    {
        if (kind == BYTE_ARRAY)
        {
            dumpArray<uint32_t>(out_, count, [=](size_t idx){return value[idx];});
        }
        else {
            OutNumber(name, value, count, kind);
        }
    }

    virtual void value(const char* name, const int64_t* value, size_t count = 1, size_t kind = 0)
    {
        if (kind == BYTE_ARRAY)
        {
            dumpArray<int64_t>(out_, count, [=](size_t idx){return value[idx];});
        }
        else {
            OutNumber(name, value, count, kind);
        }
    }

    virtual void value(const char* name, const uint64_t* value, size_t count = 1, size_t kind = 0)
    {
        if (kind == BYTE_ARRAY)
        {
            dumpArray<uint64_t>(out_, count, [=](size_t idx){return value[idx];});
        }
        else {
            OutNumber(name, value, count, kind);
        }
    }

#ifdef MMA_HAS_INT128
    virtual void value(const char* name, const UInt128T* value, size_t count = 1, size_t kind = 0)
    {
        if (kind == BYTE_ARRAY)
        {
            dumpArray<UInt128T>(out_, count, [=](size_t idx){return value[idx];});
        }
        else {
            OutNumber(name, value, count, kind);
        }
    }

    virtual void value(const char* name, const Int128T* value, size_t count = 1, size_t kind = 0)
    {
        if (kind == BYTE_ARRAY)
        {
            dumpArray<Int128T>(out_, count, [=](size_t idx){return value[idx];});
        }
        else {
            OutNumber(name, value, count, kind);
        }
    }
#endif

    virtual void value(const char* name, const float* value, size_t count = 1, size_t kind = 0)
    {
        if (kind == BYTE_ARRAY)
        {
            dumpArray<float>(out_, count, [=](size_t idx){return value[idx];});
        }
        else {
            OutNumber(name, value, count, kind);
        }
    }

    virtual void value(const char* name, const double* value, size_t count = 1, size_t kind = 0)
    {
        if (kind == BYTE_ARRAY)
        {
            dumpArray<double>(out_, count, [=](size_t idx){return value[idx];});
        }
        else {
            OutNumber(name, value, count, kind);
        }
    }


    virtual void value(const char* name, const IDValue* value, size_t count = 1, size_t kind = 0)
    {
        if (!line_)
        {
            dumpFieldHeader(out_, level_, cnt_++, name);
        }
        else {
            out_ << "    " << name << " ";
        }

        for (size_t c = 0; c < count; c++)
        {
            out_ << *value;

            if (c < count - 1)
            {
                out_ << ", ";
            }
        }

        if (!line_)
        {
            out_ << std::endl;
        }
    }

    virtual void value(const char* name, const UUID* value, size_t count = 1, size_t kind = 0)
    {
        if (kind == BYTE_ARRAY)
        {
            dumpArray<UUID>(out_, count, [=](size_t idx){return value[idx];});
        }
        else {
            OutNumber(name, value, count, kind);
        }
    }

    virtual void value(const char* name, const UID64* value, size_t count = 1, size_t kind = 0)
    {
        if (kind == BYTE_ARRAY)
        {
            dumpArray<UID64>(out_, count, [=](size_t idx){return value[idx];});
        }
        else {
            OutNumber(name, value, count, kind);
        }
    }

    virtual void value(const char* name, const UID256* value, size_t count = 1, size_t kind = 0)
    {
        if (kind == BYTE_ARRAY)
        {
            dumpArray<UID256>(out_, count, [=](size_t idx){return value[idx];});
        }
        else {
            OutNumber(name, value, count, kind);
        }
    }

    virtual void value(const char* name, const UAcc64T* value, size_t count = 1, size_t kind = 0)
    {
        if (kind == BYTE_ARRAY)
        {
            dumpArray<UAcc64T>(out_, count, [=](size_t idx){return value[idx];});
        }
        else {
            OutNumber(name, value, count, kind);
        }
    }

    virtual void value(const char* name, const UAcc128T* value, size_t count = 1, size_t kind = 0)
    {
        if (kind == BYTE_ARRAY)
        {
            dumpArray<UAcc128T>(out_, count, [=](size_t idx){return value[idx];});
        }
        else {
            OutNumber(name, value, count, kind);
        }
    }

    virtual void value(const char* name, const UAcc192T* value, size_t count = 1, size_t kind = 0)
    {
        if (kind == BYTE_ARRAY)
        {
            dumpArray<UAcc192T>(out_, count, [=](size_t idx){return value[idx];});
        }
        else {
            OutNumber(name, value, count, kind);
        }
    }

    virtual void value(const char* name, const UAcc256T* value, size_t count = 1, size_t kind = 0)
    {
        if (kind == BYTE_ARRAY)
        {
            dumpArray<UAcc256T>(out_, count, [=](size_t idx){return value[idx];});
        }
        else {
            OutNumber(name, value, count, kind);
        }
    }

    virtual void value(const char* name, const CowBlockID<UID64>* value, size_t count = 1, size_t kind = 0)
    {
        if (kind == BYTE_ARRAY)
        {
            dumpArray<CowBlockID<UID64>>(out_, count, [=](size_t idx){return value[idx];});
        }
        else {
            OutNumber(name, value, count, kind);
        }
    }

    virtual void value(const char* name, const CowBlockID<UID256>* value, size_t count = 1, size_t kind = 0)
    {
        if (kind == BYTE_ARRAY)
        {
            dumpArray<CowBlockID<UID256>>(out_, count, [=](size_t idx){return value[idx];});
        }
        else {
            OutNumber(name, value, count, kind);
        }
    }


    virtual void value(const char* name, const CowBlockID<UUID>* value, size_t count = 1, size_t kind = 0)
    {
        if (kind == BYTE_ARRAY)
        {
            dumpArray<CowBlockID<UUID>>(out_, count, [=](size_t idx){return value[idx];});
        }
        else {
            OutNumber(name, value, count, kind);
        }
    }


    virtual void symbols(const char* name, const uint64_t* value, size_t count, size_t bits_per_symbol)
    {
        dumpSymbols(out_, value, count, bits_per_symbol);
    }

    virtual void symbols(const char* name, const uint8_t* value, size_t count, size_t bits_per_symbol)
    {
        dumpSymbols(out_, value, count, bits_per_symbol);
    }


    virtual void value(const char* name, const BlockDataValueProvider& value)
    {
        if (value.isArray())
        {
            OutLine(name);
            out_ << std::endl;
            dumpPageDataValueProviderAsArray(out_, value);
        }
        else {
            OutValueInLine(name, value);
        }
    }

    virtual void as_uint8_array(const char* name, size_t count, std::function<uint8_t(size_t)> fn)
    {
        OutLine(name);
        dumpArray<uint8_t>(out_, count, fn);
    }



    virtual void startStruct() {
        out_ << std::endl;
    }

    virtual void endStruct() {}

private:

    void dumpFieldHeader(std::ostream &out, size_t level, size_t idx, U8StringRef name)
    {
        std::stringstream str;
        Expand(str, level);
        str << "FIELD: ";
        str << idx << " " << name;

        int size = str.str().size();
        Expand(str, 30 - size);
        out << str.str();
    }

    void dumpLineHeader(std::ostream &out, size_t level, size_t idx, U8StringRef name)
    {
        std::stringstream str;
        Expand(str, level);
        str << name << ": ";
        str << idx << " ";

        int size = str.str().size();
        Expand(str, 15 - size);
        out << str.str();
    }



    template <typename T>
    void OutNumber(const char* name, const T* value, size_t count, size_t kind)
    {
        if (!line_)
        {
            dumpFieldHeader(out_, level_, cnt_++, name);
        }
        else {
            out_ << "    " << name << " ";
        }

        for (size_t c = 0; c < count; c++)
        {
            out_.width(12);
            detail::OutputHelepr<T>::out(out_, *(value + c));

            //out_ << *(value + c);

            if (c < count - 1)
            {
                out_ << ",";
            }

            out_ << " ";
        }

        if (!line_)
        {
            out_ << std::endl;
        }
    }

    void OutLine(const char* name)
    {
        if (!line_)
        {
            dumpFieldHeader(out_, level_, cnt_++, name);
        }
        else {
            out_ << "    " << name << " ";
        }
    }


    void OutValueInLine(const char* name, const BlockDataValueProvider& value)
    {
        OutLine(name);

        for (size_t c = 0; c < value.size(); c++)
        {
            out_.width(24);
            out_ << value.value(c);

            if (c < value.size() - 1)
            {
                out_ << ",";
            }

            out_ << " ";
        }

        if (!line_)
        {
            out_ << std::endl;
        }
    }
};


template <typename Struct>
void DumpStruct(const Struct* s, std::ostream& out = std::cout)
{
    TextBlockDumper dumper(out);
    s->generateDataEvents(&dumper);
}

template <typename StructSO>
void DumpStruct(const StructSO& s, std::ostream& out = std::cout)
{
    TextBlockDumper dumper(out);
    s.generateDataEvents(&dumper);
}


}
