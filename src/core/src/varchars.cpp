
// Copyright 2019 Victor Smirnov
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


#include <memoria/v1/core/datatypes/varchars/varchars.hpp>

#include <memoria/v1/core/memory/malloc.hpp>
#include <memoria/v1/core/tools/bitmap.hpp>

namespace memoria {
namespace v1 {

void VarcharStorage::destroy() noexcept
{
    this->~VarcharStorage();
    free_system(this);
}

VarcharStorage* VarcharStorage::create(ViewType view)
{
    size_t storage_class_size = sizeof(VarcharStorage) | 0x7; // + up to 7 bytes of alignment

    char* ptr = allocate_system<char>(storage_class_size + view.size()).release();

    MemCpyBuffer(view.data(), ptr + storage_class_size, view.size());

    try {
        return new (ptr) VarcharStorage(ViewType{ptr + storage_class_size, view.size()});
    }
    catch (...) {
        free_system(ptr);
        throw;
    }
}

U8String VarcharStorage::to_sdn_string() const
{
    return datum_to_sdn_string(Varchar(), SelectorTag(), view_);
}


template <>
Datum<Varchar> Datum<Varchar>::from_sdn(const LDDocument& sdn_doc)
{
    LDDValueView value = sdn_doc.value();
    if (value.is_varchar()) {
        return Datum<Varchar>(value.as_varchar().view());
    }
    else if (value.is_double()) {
        return Datum<Varchar>(std::to_string(value.as_double()));
    }
    else if (value.is_bigint()) {
        return Datum<Varchar>(std::to_string(value.as_bigint()));
    }

    else if (value.is_boolean()) {
        return Datum<Varchar>(value.as_boolean() ? "true" : "false");
    }

    MMA1_THROW(RuntimeException())
                << fmt::format_ex(
                       u"Unsupported data type requested: {}", value.to_standard_string()
                       );

}

}}
