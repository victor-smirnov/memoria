
// Copyright 2022 Victor Smirnov
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


#include <memoria/core/hermes/array/array_ext.hpp>

namespace memoria::hermes {

using GAPoolT = pool::SimpleObjectPool<TypedGenericArray<Object>>;
using GAPoolPtrT = boost::local_shared_ptr<GAPoolT>;

PoolSharedPtr<GenericArray> TypedGenericArray<Object>::make_wrapper(LWMemHolder* ctr_holder, void* array) {
    static thread_local GAPoolPtrT wrapper_pool = MakeLocalShared<GAPoolT>();
    return wrapper_pool->allocate_shared(ctr_holder, array);
}

}
