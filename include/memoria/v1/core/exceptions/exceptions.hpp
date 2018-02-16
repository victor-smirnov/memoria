
// Copyright 2011 Victor Smirnov
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

#include <memoria/v1/core/exceptions/base.hpp>
#include <memoria/v1/core/exceptions/core.hpp>

#ifdef _WIN32
#include <memoria/v1/core/exceptions/win32_system.hpp>
#else
#include <memoria/v1/core/exceptions/linux_system.hpp>
#endif
