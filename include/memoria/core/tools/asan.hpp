
// Copyright 2017 Victor Smirnov
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

#ifdef MEMORIA_USE_ASAN
#include <sanitizer/asan_interface.h>

#ifdef MMA_SANITIZE_STACKS

#ifdef __clang__
#   if (__clang_major__ == 3 && __clang_minor__ >= 9) 
#       define MMA_SANITIZE_STACKS 1
#       define MMA_START_SWITCH_FIBER(fake_stack_save, stack_bottom, stack_size) \
            __sanitizer_start_switch_fiber(&fake_stack_save, stack_bottom, stack_size)
#       define MMA_FINISH_SWITCH_FIBER(fake_stack_save, stack_bottom, stack_size) \
            __sanitizer_finish_switch_fiber(fake_stack_save); 
#   elif __clang_major__ >= 4
#       define MMA_SANITIZE_STACKS 1
#       define MMA_START_SWITCH_FIBER(fake_stack_save, stack_bottom, stack_size) \
            __sanitizer_start_switch_fiber(&fake_stack_save, stack_bottom, stack_size)
#       define MMA_FINISH_SWITCH_FIBER(fake_stack_save, stack_bottom, stack_size) \
            __sanitizer_finish_switch_fiber(fake_stack_save, &stack_bottom, &stack_size)
#   else
#       define MMA_START_SWITCH_FIBER(fake_stack_save, stack_bottom, stack_size)
#       define MMA_FINISH_SWITCH_FIBER(fake_stack_save, stack_bottom, stack_size)
#   endif
#elif defined(__GNUC__)
#   if __GNUC__ >= 7       
#       define MMA_SANITIZE_STACKS 1
#       define MMA_START_SWITCH_FIBER(fake_stack_save, stack_bottom, stack_size) \
            __sanitizer_start_switch_fiber(&fake_stack_save, stack_bottom, stack_size)
#       define MMA_FINISH_SWITCH_FIBER(fake_stack_save, stack_bottom, stack_size) \
            __sanitizer_finish_switch_fiber(fake_stack_save, &stack_bottom, &stack_size)
#   else
#       define MMA_START_SWITCH_FIBER(fake_stack_save, stack_bottom, stack_size)
#       define MMA_FINISH_SWITCH_FIBER(fake_stack_save, stack_bottom, stack_size)
#   endif
#else
#define MMA_START_SWITCH_FIBER(fake_stack_save, stack_bottom, stack_size)
#define MMA_FINISH_SWITCH_FIBER(fake_stack_save, stack_bottom, stack_size)
#endif

#else
#define MMA_START_SWITCH_FIBER(fake_stack_save, stack_bottom, stack_size)
#define MMA_FINISH_SWITCH_FIBER(fake_stack_save, stack_bottom, stack_size)
#endif            

#else

#define MMA_START_SWITCH_FIBER(fake_stack_save, stack_bottom, stack_size)
#define MMA_FINISH_SWITCH_FIBER(fake_stack_save, stack_bottom, stack_size)

#endif
