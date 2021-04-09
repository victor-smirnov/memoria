
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


#include <memoria/profiles/impl/cow_profile.hpp>
#include <memoria/profiles/impl/no_cow_profile.hpp>
#include <memoria/profiles/impl/cow_lite_profile.hpp>
#include <memoria/memoria_ctrs.hpp>

namespace memoria {


void InitMemoriaCtrsExplicit() {
    InitCtrDatatypes();

    StaticLibraryCtrs<CowLiteProfile<>>::init();

    StaticLibraryCtrs<NoCowProfile<>>::init();

    StaticLibraryCtrs<CowProfile<>>::init();
}

}
