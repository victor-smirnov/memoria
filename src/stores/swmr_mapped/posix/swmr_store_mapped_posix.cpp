
// Copyright 2021 Victor Smirnov
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

#include <memoria/profiles/impl/memory_cow_profile.hpp>
#include <memoria/store/swmr/mapped/swmr_mapped_store.hpp>

#include <mutex>
#include <unordered_map>
#include <sys/file.h>
#include <fcntl.h>

using namespace memoria;

using boost::interprocess::file_handle_t;

memoria::detail::FileLockHandler::~FileLockHandler() noexcept {}

namespace {

using GuardT = std::lock_guard<std::mutex>;

struct LockStatus {
    bool locked;
};

class FileHandleThreadLocks {
    std::mutex mtx_;
    std::unordered_map<file_handle_t, LockStatus> locks_;

public:
    std::mutex& mtx() noexcept {
        return mtx_;
    }

    bool try_lock_for(file_handle_t handle) {
        auto ii = locks_.find(handle);
        if (ii == locks_.end()) {
            locks_[handle] = LockStatus{true};
            return true;
        }
        else {
            return false;
        }
    }

    void remove_lock(file_handle_t handle) {
        locks_.erase(handle);
    }
};

class PosixFileLockImpl: public detail::FileLockHandler {
    std::string name_;
    file_handle_t handle_;
    FileHandleThreadLocks& locks_;
public:
    PosixFileLockImpl(const char* name, file_handle_t handle, FileHandleThreadLocks& locks):
        name_(name),
        handle_(handle), locks_(locks) {}

    ~PosixFileLockImpl() noexcept {
        if (handle_ >= 0)
        {
            
            flock(handle_, LOCK_UN);
            close(handle_);
        }
    }

    VoidResult unlock() noexcept {
        return wrap_throwing([&]() -> VoidResult {
            GuardT guard(locks_.mtx());
            locks_.remove_lock(handle_);
            int res = flock(handle_, LOCK_UN);
            if (res) {
                auto err = make_generic_error("Can't UNLOCK an SWMRStore file: {}, reason: {}", name_, strerror(errno));
                close(handle_);
                handle_ = -1;
                return err;
            }

            close(handle_);
            handle_ = -1;

            return VoidResult::of();
        });
    }
};


using ResultT = Result<std::unique_ptr<detail::FileLockHandler>>;

}

ResultT detail::FileLockHandler::lock_file(const char* name, bool create_file) noexcept {
    static FileHandleThreadLocks locks;

    return wrap_throwing([&]() -> ResultT {
        GuardT guard(locks.mtx());

        file_handle_t handle;
        if (create_file) {
            handle = open64(name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
        }
        else {
            handle = open64(name, O_RDWR);
        }
        if (handle < 0) {
            return make_generic_error("Cant open file {} for locking", name);
        }

        if (locks.try_lock_for(handle)) {
            int res = flock(handle, LOCK_EX | LOCK_NB);

            if (res == 0) {
                return ResultT::of(std::make_unique<PosixFileLockImpl>(name, handle, locks));
            }
            else if (res == EWOULDBLOCK) {
                close(handle);
                return ResultT::of();
            }
            else {
                close(handle);
                return make_generic_error("Can't LOCK an SWMRStore file: {}, reason: {}", name, strerror(errno));
            }
        }
        else {
            // the file has been already locked in this process
            close(handle);
            return ResultT::of();
        }
    });
}
