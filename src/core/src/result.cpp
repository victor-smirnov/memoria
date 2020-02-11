
// Copyright 2020 Victor Smirnov
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


#include <memoria/core/tools/result.hpp>

namespace memoria {

void release(MemoriaError* error) noexcept {
    if (error) {
        error->release();
    }
}

void WrappedExceptionMemoriaError::release() noexcept {
    delete this;
}

void WrappedExceptionMemoriaError::describe(std::ostream& out) const noexcept
{
    try {
        std::rethrow_exception(ptr_);
    }
    catch (MemoriaThrowable& mth) {
        mth.dump(out);
    }
    catch (std::exception& ex) {
        out << boost::diagnostic_information(ex);
    }
    catch (boost::exception& ex) {
        out << boost::diagnostic_information(ex);
    }
    catch (...) {
        out << "Unknown Exception";
    }
}

const U8String WrappedExceptionMemoriaError::what() const noexcept
{
    return "WrappedExceptionMemoriaError";
}

std::exception_ptr& WrappedExceptionMemoriaError::ptr() {
    return ptr_;
}

void WrappedExceptionMemoriaError::rethrow() const {
    std::rethrow_exception(ptr_);
}


MemoriaErrorPtr WrappedExceptionMemoriaError::create(std::exception_ptr&& ptr)
{
    return make_memoria_error<WrappedExceptionMemoriaError>(std::move(ptr));
}

}
