
// Copyright 2018 Victor Smirnov
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

#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/strings/string.hpp>
#include <memoria/core/strings/format.hpp>
#include <memoria/core/tools/ptr_cast.hpp>

#include <unicode/utext.h>
#include <cstring>


#define I32_FLAG(bitIndex) ((int32_t)1<<(bitIndex))

namespace memoria {

struct UTextUCharFuncsHolder {
    const UTextFuncs* funcs_;

    UTextUCharFuncsHolder()
    {
        UChar array[] = {0};

        UText ut = UTEXT_INITIALIZER;
        UErrorCode status = U_ZERO_ERROR;
        utext_openUChars(&ut, array, 0, &status);
        StringException::assertOk(status);

        funcs_ = ut.pFuncs;

        utext_close(&ut);
    }
};

const UTextFuncs* get_uchars_utext_funcs()
{
    thread_local static UTextUCharFuncsHolder utext_funcs{};
    return utext_funcs.funcs_;
}


void adjust_ptr(UText *dest, const void **dest_ptr, const UText *src)
{
    const char *dptr = T2T<char*>(*dest_ptr);
    char *d_utext = T2T<char*>(dest);
    char *s_utext = T2T<char*>(src);

    const char* src_pExtra = T2T<const char*>(src->pExtra);
    char* dest_pExtra = T2T<char*>(dest->pExtra);

    if (dptr >= src_pExtra && dptr < (src_pExtra + src->extraSize))
    {
        *dest_ptr = dest_pExtra + (dptr - src_pExtra);
    }
    else if (dptr >= s_utext && dptr < s_utext + src->sizeOfStruct) {
        *dest_ptr = d_utext + (dptr - s_utext);
    }
}



UText *
shallow_text_clone(UText * dest, const UText * src, UErrorCode * status)
{
    if (U_FAILURE(*status)) {
        return nullptr;
    }

    dest = utext_setup(dest, src->extraSize, status);
    if (U_FAILURE(*status)) {
        return dest;
    }


    void *dest_extra = dest->pExtra;
    int32_t flags   = dest->flags;


    int size_to_copy = src->sizeOfStruct;
    if (size_to_copy > dest->sizeOfStruct)
    {
        size_to_copy = dest->sizeOfStruct;
    }

    memcpy(dest, src, size_to_copy);
    dest->pExtra = dest_extra;
    dest->flags  = flags;
    if (src->extraSize > 0)
    {
        memcpy(dest->pExtra, src->pExtra, src->extraSize);
    }

    // Relocate pointers in dest still looking into buffers in src.
    adjust_ptr(dest, &dest->context, src);
    adjust_ptr(dest, &dest->p, src);
    adjust_ptr(dest, &dest->q, src);
    adjust_ptr(dest, &dest->r, src);
    adjust_ptr(dest, tools::ptr_cast<const void*>(&dest->chunkContents), src); //(const void **)

    // Not actually needed here, but let's do it anyway.
    dest->providerProperties &= ~I32_FLAG(UTEXT_PROVIDER_OWNS_TEXT);

    return dest;
}




template <typename PtrT>
void u16string_close(UText *ut)
{
    PtrT* shared_ptr = T2T<PtrT*>(ut->q);
    shared_ptr->~PtrT();
    ut->context = nullptr;
    ut->q = nullptr;
}


template <typename PtrT>
static UText * u16string_clone(UText *dest, const UText * src, UBool deep, UErrorCode* status)
{
    if (U_FAILURE(*status)) {
        return nullptr;
    }

    if (!deep)
    {
        dest = shallow_text_clone(dest, src, status);

        if (U_SUCCESS(*status))
        {
            PtrT* existing_string_ptr = T2T<PtrT*>(src->q);
            dest->q = new PtrT(*existing_string_ptr);
        }
    }
    else {
        *status = U_UNSUPPORTED_ERROR;
    }

    return dest;
}



template <typename StrT>
struct UTextUStringFuncsHolder {
    UTextFuncs funcs_;

    UTextUStringFuncsHolder(): funcs_{*get_uchars_utext_funcs()}
    {
        funcs_.close = u16string_close<StrT>;
        funcs_.clone = u16string_clone<StrT>;
    }
};


UTextUniquePtr make_utext(const U16String& str)
{
    using PtrT = std::shared_ptr<U16String>;
    static thread_local UTextUStringFuncsHolder<PtrT> funcs;

    auto ptr = new PtrT(std::make_shared<U16String>(str));

    std::unique_ptr<UText> utp = std::make_unique<UText>();
    *utp = UTEXT_INITIALIZER;


    UErrorCode status = U_ZERO_ERROR;
    utext_openUChars(utp.get(), castChars((*ptr)->data()), (*ptr)->size(), &status);

    StringException::assertOk(status);

    utp->pFuncs = &funcs.funcs_;

    utp->q = ptr;

    return UTextUniquePtr(utp.release(), [](UText* ut0) {
        utext_close(ut0);
        delete ut0;
    });
}

UTextUniquePtr make_utext(U16String&& str)
{
    using PtrT = std::shared_ptr<U16String>;
    static thread_local UTextUStringFuncsHolder<PtrT> funcs;

    auto ptr = new PtrT(std::make_shared<U16String>(std::move(str)));

    std::unique_ptr<UText> utp = std::make_unique<UText>();
    *utp = UTEXT_INITIALIZER;

    UErrorCode status = U_ZERO_ERROR;
    utext_openUChars(utp.get(), castChars((*ptr)->data()), (*ptr)->size(), &status);

    StringException::assertOk(status);

    utp->pFuncs = &funcs.funcs_;

    utp->q = ptr;

    return UTextUniquePtr(utp.release(), [](UText* ut0) {
        utext_close(ut0);
        delete ut0;
    });
}


UTextUniquePtr make_utext_ref(U16String& str)
{
    std::unique_ptr<UText> utp = std::make_unique<UText>();
    *utp = UTEXT_INITIALIZER;

    UErrorCode status = U_ZERO_ERROR;
    utext_openUChars(utp.get(), castChars(str.data()), str.size(), &status);

    StringException::assertOk(status);

    return UTextUniquePtr(utp.release(), [](UText* ut0) {delete ut0;});
}

static void extract_data_from_ut(UText* ut, int64_t start, int64_t limit, char16_t* buffer, size_t buffer_size)
{
    with_icu_error([&](UErrorCode* status) {
        int64_t block_size = 1024*1024;

        int64_t buffer_offset{};
        while (start < limit)
        {
            int64_t to_read = (limit - start <= block_size) ? block_size : (limit - start);

            int64_t processed = utext_extract(ut, start, limit, castChars(buffer + buffer_offset), to_read, status);

            if (U_FAILURE(*status)) {
                break;
            }

            start += processed;
            buffer_offset += processed;
        }
    });
}

U16String to_u16string(UText* utext)
{
    int64_t native_length = utext_nativeLength(utext);
    int64_t length{};

    if (!utext->pFuncs->mapNativeIndexToUTF16)
    {
        length = native_length;
    }
    else {
        utext->pFuncs->access(utext, native_length, true);

        length = utext->pFuncs->mapOffsetToNative(utext);
    }

    U16String str(length, u'\u0000');

    extract_data_from_ut(utext, 0, native_length, str.data(), length);

    return str;
}


U16String to_u16string(UText* utext, int64_t start, int64_t length)
{
    int64_t native_length = utext_nativeLength(utext);

    int64_t cu16_start{};
    int64_t cu16_limit{};

    int64_t limit = (start + length) <= native_length ? (start + length) : native_length;
    if (start < 0) start = 0;

    if (limit > start)
    {
        // The string is within chunkContents
        if (start >= utext->chunkNativeStart && limit <= utext->chunkNativeLimit)
        {
            int32_t start_chunk_offset = get_chunk_index_for(utext, start);
            U16_SET_CP_START(utext->chunkContents, 0, start_chunk_offset);


            int32_t limit_chunk_offset = get_chunk_index_for(utext, limit);
            if (limit_chunk_offset < utext->chunkLength - 1)
            {
                U16_SET_CP_START(utext->chunkContents, 0, start_chunk_offset);
            }

            int64_t cu16_length = limit_chunk_offset - start_chunk_offset;
            return U16String(T2T<const char16_t*>(utext->chunkContents + start_chunk_offset), cu16_length);
        }
        else {
            if (!utext->pFuncs->mapNativeIndexToUTF16)
            {
                utext->pFuncs->access(utext, start, false);
                cu16_start = utext->chunkNativeStart + utext->chunkOffset;

                utext->pFuncs->access(utext, limit, false);
                cu16_limit = limit;
            }
            else {
                utext->pFuncs->access(utext, start, false);
                cu16_limit = utext->pFuncs->mapOffsetToNative(utext);
            }

            int64_t cu16_length = cu16_limit - cu16_start;
            U16String str(cu16_length, u'\u0000');

            extract_data_from_ut(utext, cu16_start, cu16_limit, str.data(), cu16_length);

            return str;
        }
    }
    else {
        return U16String();
    }
}



class UTextCU16Provider: public ICodeUnit16Provider {
    UTextUniquePtr utext_;
public:
    UTextCU16Provider(UTextUniquePtr&& utext): utext_(std::move(utext))
    {}


    virtual int64_t read_to(int64_t position, char16_t* data, size_t size)
    {
        UErrorCode status = U_ZERO_ERROR;
        int64_t processed = utext_extract(utext_.get(), position, size, T2T<UChar*>(data), size, &status);
        StringException::assertOk(status);
        return processed;
    }

    virtual int64_t length() const {
        return utext_nativeLength(utext_.get());
    }

    virtual int64_t align_to_code_unit_start(int64_t position)
    {
        if (position < 0) {
            return 0;
        }
        else {
            int64_t str_length = length();

            if (position < str_length)
            {
                if (utext_->pFuncs->access(utext_.get(), position, true))
                {
                    if (!utext_->pFuncs->mapOffsetToNative) {
                        return utext_->chunkNativeStart + utext_->chunkOffset;
                    }
                    else {
                        return utext_->pFuncs->mapOffsetToNative(utext_.get());
                    }
                }
                else {
                    MMA1_THROW(Exception()) << WhatInfo(format_u8("Can't access UText at {}", position));
                }
            }
            else {
                return str_length;
            }
        }
    }

    virtual std::shared_ptr<ICodeUnit16Provider> clone() const
    {
        std::unique_ptr<UText> utp = std::make_unique<UText>();
        *utp = UTEXT_INITIALIZER;

        UErrorCode status = U_ZERO_ERROR;
        utext_clone(utp.get(), utext_.get(), false, true, &status);
        StringException::assertOk(status);

        UTextUniquePtr clone_ptr(utp.release(), [](UText* ut) {
            utext_close(ut);
            delete ut;
        });

        return as_cu16_provider(std::move(clone_ptr));
    }
};


CU16ProviderPtr as_cu16_provider(UTextUniquePtr&& utext_ptr)
{
    return std::static_pointer_cast<ICodeUnit16Provider>(std::make_shared<UTextCU16Provider>(std::move(utext_ptr)));
}


class U16StringCU16Provider: public ICodeUnit16Provider {
    std::shared_ptr<U16String> str_;
public:
    U16StringCU16Provider(U16String&& str): str_(std::make_shared<U16String>(std::move(str)))
    {}

    virtual int64_t read_to(int64_t position, char16_t* data, size_t size)
    {
        int64_t limit = position + (int64_t)size;
        if (position < 0) {
            position = 0;
        }

        if (limit > length()) {
            limit = length();
        }

        int64_t length = limit - position;

        if (length > 0) {
            std::memmove(data, str_->data() + position, length * sizeof(char16_t));
        }

        return length;
    }

    virtual int64_t length() const {
        return str_->size();
    }

    virtual int64_t align_to_code_unit_start(int64_t position)
    {
        if (position < 0) {
            return 0;
        }
        else {
            int64_t str_length = length();

            if (position < str_length)
            {
                U16_SET_CP_START(str_->data(), 0, position);
                return position;
            }
            else {
                return str_length;
            }
        }
    }

    virtual std::shared_ptr<ICodeUnit16Provider> clone() const
    {
        return std::static_pointer_cast<ICodeUnit16Provider>(std::make_shared<U16StringCU16Provider>(*this));
    }
};

CU16ProviderPtr as_cu16_provider(U16String&& str)
{
    return std::static_pointer_cast<ICodeUnit16Provider>(std::make_shared<U16StringCU16Provider>(std::move(str)));
}

int64_t get_native_index_for(UText* ut)
{
    if (!ut->pFuncs->mapOffsetToNative)
    {
        return ut->chunkNativeStart + ut->chunkOffset;
    }
    else {
        return ut->chunkNativeStart + ut->pFuncs->mapOffsetToNative(ut);
    }
}



int64_t get_chunk_index_for(UText* ut, int64_t native_index)
{
    if (!ut->pFuncs->mapNativeIndexToUTF16)
    {
        return native_index - ut->chunkNativeStart;
    }
    else {
        return ut->pFuncs->mapNativeIndexToUTF16(ut, native_index);
    }
}

int64_t get_u16_index_for(UText* ut, int64_t native_index)
{
    return get_chunk_index_for(ut, native_index) + ut->chunkNativeStart;
}

}
