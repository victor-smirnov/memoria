
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

#include <memoria/core/regexp/icu_regexp.hpp>
#include <memoria/core/exceptions/exceptions.hpp>

#ifndef MMA1_NO_REACTOR
#   include <memoria/reactor/reactor.hpp>
#endif


#include <unicode/utext.h>
#include <unicode/utypes.h>
#include <unicode/unistr.h>
#include <unicode/utf.h>
#include <unicode/utf16.h>

#include <iostream>


namespace memoria {

static int64_t pin_index(int64_t &index, int64_t limit)
{
    if (index < 0) {
        index = 0;
    }
    else if (index > limit) {
        index = limit;
    }
    return index;
}

U_CAPI int64_t
terminate_uchars(UChar *dest, int64_t dest_capacity, int64_t length, UErrorCode* p_error_code)
{
    if (p_error_code && U_SUCCESS(*p_error_code))
    {
        if (length < 0)
        {}
        else if (length < dest_capacity)
        {
            dest[length] = 0;
            *p_error_code = U_ZERO_ERROR;
        }
        else if (length == dest_capacity) {
            *p_error_code = U_STRING_NOT_TERMINATED_WARNING;
        }
        else {
            *p_error_code = U_BUFFER_OVERFLOW_ERROR;
        }
    }

    return length;
}

//------------------------------------------------------------------------------
//
//     UText implementation for text from ICU CharacterIterators
//
//         Use of UText data members:
//            context    pointer to the CharacterIterator
//            a          length of the full text.
//            p          pointer to  buffer 1
//            b          start index of local buffer 1 contents
//            q          pointer to buffer 2
//            c          buffer_size
//
//------------------------------------------------------------------------------

static void U_CALLCONV
codeunit_iterator_text_close(UText *ut)
{
    if (ut->context)
    {
        CU16ProviderPtr* iter_ptr = T2T<CU16ProviderPtr*>(ut->context);
        delete iter_ptr;
        ut->context = nullptr;
    }
}

static int64_t U_CALLCONV
codeunit_iterator_text_length(UText *ut) {
    return ut->a;
}



static int64_t codeuint_iterator_access_inbound(UText* ut, int64_t index, int64_t length, char16_t* buf)
{
    CU16ProviderPtr& iter = *T2T<CU16ProviderPtr*>(ut->context);

    int64_t buffer_size = ut->c - 1;

    int64_t cp_index = iter->align_to_code_unit_start(index);

    int64_t processed{};
    if ((cp_index + buffer_size) <= length)
    {
        processed = iter->read_to(cp_index, buf, buffer_size);
    }
    else {
        processed = iter->read_to(cp_index, buf, length - cp_index);
    }

    if (processed > 0)
    {
        if (U_IS_SURROGATE(*(buf + processed - 1))) {
            processed--;
        }

        if (processed > 0)
        {
            ut->chunkOffset = 0;
            ut->chunkLength = processed;
            ut->chunkNativeStart = cp_index;
            ut->chunkNativeLimit = processed;
            ut->nativeIndexingLimit = processed;

            if (processed <= buffer_size) {
                buf[processed] = 0;
            }

            return cp_index;
        }
    }

    return cp_index;
}


static UBool U_CALLCONV
codeunit_iterator_text_access_forward(UText *ut, int64_t index)
{
    int64_t length = ut->a;

    char16_t* buf = T2T<char16_t*>(ut->pExtra);

    int64_t chunk_native_start{};

    if (index >= 0 && index < length)
    {
        chunk_native_start = codeuint_iterator_access_inbound(ut, index, length, buf);
        return true;
    }

    buf[0] = 0;

    ut->chunkOffset = 0;
    ut->chunkLength = 0;
    ut->chunkNativeStart = chunk_native_start;
    ut->chunkNativeLimit = 0;
    ut->nativeIndexingLimit = 0;

    return false;
}


static UBool U_CALLCONV
codeunit_iterator_text_access_backward(UText *ut, int64_t index)
{
    int64_t length = ut->a;
    char16_t* buf = T2T<char16_t*>(ut->pExtra);

    int64_t chunk_native_start{};

    if (index >= 0 && index <= length)
    {
        chunk_native_start = codeuint_iterator_access_inbound(ut, index, length, buf);
        return true;
    }

    buf[0] = 0;

    ut->chunkOffset = 0;
    ut->chunkLength = 0;
    ut->chunkNativeStart = chunk_native_start;
    ut->chunkNativeLimit = 0;
    ut->nativeIndexingLimit = 0;

    return false;
}



static UBool U_CALLCONV
codeunit_iterator_text_access(UText *ut, int64_t index, UBool forward)
{
    try {
        if (forward)
        {
            if (index > ut->chunkNativeStart && index < ut->chunkNativeLimit)
            {
                ut->chunkOffset = get_chunk_index_for(ut, index);

                U16_SET_CP_START(ut->chunkContents, 0, ut->chunkOffset);
                return true;
            }
            else {
                return codeunit_iterator_text_access_forward(ut, index);
            }
        }
        else {
            if (index > ut->chunkNativeStart && index < ut->chunkNativeLimit)
            {
                int64_t chunk_offset = get_chunk_index_for(ut, index);
                U16_SET_CP_START(ut->chunkContents, 0, chunk_offset);

                if (chunk_offset > 0)
                {
                    ut->chunkOffset = chunk_offset;
                    return true;
                }
                else {
                    return codeunit_iterator_text_access_backward(ut, index);
                }
            }
            else {
                return codeunit_iterator_text_access_backward(ut, index);
            }
        }
    }
    catch (MemoriaThrowable& ex) {
        ex.dump(std::cout);
        return false;
    }
    catch (...) {
        return false;
    }
}


static UText * U_CALLCONV
codeunit_iterator_text_clone(UText *dest, const UText *src, UBool deep, UErrorCode * status)
{
    if (U_FAILURE(*status)) {
        return nullptr;
    }

    if (!deep)
    {
        CU16ProviderPtr& iter = *T2T<CU16ProviderPtr*>(src->context);

        try {
            auto src_iter = iter->clone();

            dest = utext_open_codepoint_accessor(dest, src_iter, src->c, status);
            if (U_FAILURE(*status)) {
                return dest;
            }

            int64_t ix = utext_getNativeIndex(const_cast<UText *>(src));
            utext_setNativeIndex(dest, ix);

            return dest;
        }
        catch (...) {
            *status = U_INTERNAL_PROGRAM_ERROR;
            return nullptr;
        }
    }
    else {
        *status = U_UNSUPPORTED_ERROR;
        return NULL;
    }
}

static int32_t U_CALLCONV
codeunit_iterator_text_extract(
                  UText *ut,
                  int64_t start, int64_t limit,
                  UChar *dest, int32_t dest_capacity,
                  UErrorCode *status)
{
    if(U_FAILURE(*status)) {
        return 0;
    }

    if(dest_capacity < 0 || (dest == nullptr && dest_capacity > 0) || start > limit)
    {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    int64_t  length  = ut->a;
    int64_t  start32 = pin_index(start, length);
    int64_t  limit32 = pin_index(limit, length);

    CU16ProviderPtr& iter_ptr = *T2T<CU16ProviderPtr*>(ut->context);

    try {

        int64_t srci = iter_ptr->align_to_code_unit_start(start32);

        int64_t requested = limit32 - srci;

        int64_t processed{};
        int64_t extra{};

        if (requested <= dest_capacity)
        {
            processed = iter_ptr->read_to(srci, T2T<char16_t*>(dest), requested);
        }
        else {
            processed = iter_ptr->read_to(srci, T2T<char16_t*>(dest), dest_capacity);

            if (dest_capacity == processed) {
                extra = requested - processed;
            }
        }

        codeunit_iterator_text_access(ut, srci + processed, TRUE);
        terminate_uchars(dest, dest_capacity, srci + processed, status);

        return processed + extra;
    }
    catch (...) {
        *status = U_INTERNAL_PROGRAM_ERROR;
        return -1;
    }
}

static const struct UTextFuncs charIterFuncs =
{
    sizeof(UTextFuncs),
    0, 0, 0,                // Reserved alignment padding
    codeunit_iterator_text_clone,
    codeunit_iterator_text_length,
    codeunit_iterator_text_access,
    codeunit_iterator_text_extract,
    nullptr,                // Replace
    nullptr,                // Copy
    nullptr,                // MapOffsetToNative,
    nullptr,                // MapIndexToUTF16,
    codeunit_iterator_text_close,
    nullptr,                // spare 1
    nullptr,                // spare 2
    nullptr                 // spare 3
};



U_CAPI UText* U_EXPORT2
utext_open_codepoint_accessor(UText *ut, const CU16ProviderPtr& ci, int32_t buffer_size, UErrorCode *status)
{
    if (U_FAILURE(*status)) {
        return nullptr;
    }

    int32_t extra_space = buffer_size * sizeof(UChar);
    ut = utext_setup(ut, extra_space, status);

    if (U_SUCCESS(*status))
    {
        ut->pFuncs               = &charIterFuncs;
        ut->context              = new CU16ProviderPtr(ci);
        ut->providerProperties   = 0;
        ut->a                    = ci->length();
        ut->p                    = ut->pExtra;
        ut->b                    = -1;
        ut->c                    = buffer_size;

        ut->chunkContents        = T2T<UChar *>(ut->pExtra);
        ut->chunkNativeStart     = -1;
        ut->chunkOffset          = 1;
        ut->chunkNativeLimit     = 0;
        ut->chunkLength          = 0;
        ut->nativeIndexingLimit  = ut->chunkOffset;
    }

    return ut;
}


}
