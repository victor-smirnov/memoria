
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


#pragma once

#include <memoria/store/lmdb/lmdb_store_readonly_commit.hpp>
#include <memoria/store/lmdb/lmdb_store_writable_commit.hpp>

#include <memoria/core/tools/span.hpp>
#include <memoria/core/memory/ptr_cast.hpp>

#include <mutex>
#include <unordered_set>
#include <functional>

#include <mdb/mma_lmdb.h>

namespace memoria {

template <typename Profile>
class LMDBStore:
    public ILMDBStore<ApiProfile<Profile>>,
    public EnableSharedFromThis<LMDBStore<Profile>>
{
    using Base = ILMDBStore<ApiProfile<Profile>>;
    using typename Base::ReadOnlyCommitPtr;
    using typename Base::WritableCommitPtr;
    using typename Base::CommitID;
    using typename Base::SequenceID;

    using ApiProfileT = ApiProfile<Profile>;

    using LMDBStoreReadOnlyCommitPtr = SharedPtr<LMDBStoreReadOnlyCommit<Profile>>;
    using LMDBStoreWritableCommitPtr = SharedPtr<LMDBStoreWritableCommit<Profile>>;
    using LMDBStoreWritableCommitWeakPtr = WeakPtr<LMDBStoreWritableCommit<Profile>>;

    using BlockID = ProfileBlockID<Profile>;

    struct CounterStorageT {
        BlockID block_id;
        uint64_t counter;
    };

    static constexpr size_t  BASIC_BLOCK_SIZE = 4096 - 64;
    static constexpr size_t  MB = 1024*1024;

    using LockGuard     = std::lock_guard<std::recursive_mutex>;
    using Superblock    = LMDBSuperblock<Profile>;

    template <typename> friend class LMDBStoreReadonlyCommit;
    template <typename> friend class LMDBStoreWritableCommit;
    template <typename> friend class LMDBStoreCommitBase;

    friend Result<SharedPtr<ILMDBStore<ApiProfileT>>> open_lmdb_store(U8StringView);
    friend Result<SharedPtr<ILMDBStore<ApiProfileT>>> open_lmdb_store_readonly(U8StringView);
    friend Result<SharedPtr<ILMDBStore<ApiProfileT>>> create_lmdb_store(U8StringView, uint64_t);

    Superblock* superblock_{};

    U8String file_name_;
    MDB_env* mdb_env_{};
    MDB_dbi system_db_{};
    MDB_dbi data_db_{};

    mutable std::recursive_mutex store_mutex_;
public:
    // create store
    LMDBStore(MaybeError& maybe_error, U8String file_name, uint64_t file_size_mb):
        file_name_(file_name)
    {
        wrap_construction(maybe_error, [&]() -> VoidResult {
            if (int rc = mma_mdb_env_create(&mdb_env_)) {
                return make_generic_error("Can't create LMDB environment, error = {}", mma_mdb_strerror(rc));
            }

            if (int rc = mma_mdb_env_set_maxdbs(mdb_env_, 50)) {
                mma_mdb_env_close(mdb_env_);
                return make_generic_error("Can't set LMDB number of databases in the environment, error = {}", mma_mdb_strerror(rc));
            }

            if (int rc = mma_mdb_env_set_maxreaders(mdb_env_, 1024)) {
                mma_mdb_env_close(mdb_env_);
                return make_generic_error("Can't set LMDB maximum number of readers in the environment to 1024, error = {}", mma_mdb_strerror(rc));
            }

            if (int rc = mma_mdb_env_set_mapsize(mdb_env_, MB * file_size_mb)) { // 1MB * 100000
                mma_mdb_env_close(mdb_env_);
                return make_generic_error("Can't set LMDB's maximal file size in the environment, error = {}", mma_mdb_strerror(rc));
            }

            if (int rc = mma_mdb_env_open(mdb_env_, file_name_.data(), MDB_NOSUBDIR | MDB_CREATE | MDB_NOTLS, 0664)) { //MDB_NOSYNC
                mma_mdb_env_close(mdb_env_);
                return make_generic_error("Can't create LMDB database {}, error = {}", file_name_, mma_mdb_strerror(rc));
            }

            MDB_txn* transaction{};

            if (const int rc = mma_mdb_txn_begin(mdb_env_, nullptr, 0, &transaction)) {
                mma_mdb_env_close(mdb_env_);
                return make_generic_error("Can't start read-write transaction, error = {}", mma_mdb_strerror(rc));
            }

            if (const int rc = mma_mdb_dbi_open(transaction, "memoria_system", MDB_CREATE, &system_db_)) {
                mma_mdb_txn_abort(transaction);
                mma_mdb_env_close(mdb_env_);
                return make_generic_error("Can't create memoria_system database, error = {}", mma_mdb_strerror(rc));
            }

            if (const int rc = mma_mdb_dbi_open(transaction, "memoria_data", MDB_CREATE, &data_db_)) {
                mma_mdb_dbi_close(mdb_env_, system_db_);
                mma_mdb_txn_abort(transaction);
                mma_mdb_env_close(mdb_env_);
                return make_generic_error("Can't create memoria_data database, error = {}", mma_mdb_strerror(rc));
            }

            superblock_ = ptr_cast<Superblock>(::malloc(BASIC_BLOCK_SIZE));
            MEMORIA_TRY_VOID(superblock_->init(BASIC_BLOCK_SIZE));

            if (const int rc = mma_mdb_txn_commit(transaction)) {
                mma_mdb_dbi_close(mdb_env_, system_db_);
                mma_mdb_dbi_close(mdb_env_, data_db_);
                mma_mdb_txn_abort(transaction);
                mma_mdb_env_close(mdb_env_);
                return make_generic_error("Can't commit transaction, error = {}", mma_mdb_strerror(rc));
            }

            return VoidResult::of();
        });
    }

    // open store
    LMDBStore(MaybeError& maybe_error, U8String file_name, bool read_only):
        file_name_(file_name)
    {
        wrap_construction(maybe_error, [&]() -> VoidResult {
            if (int rc = mma_mdb_env_create(&mdb_env_)) {
                return make_generic_error("Can't create LMDB environment");
            }

            if (int rc = mma_mdb_env_set_maxdbs(mdb_env_, 50)) {
                mma_mdb_env_close(mdb_env_);
                return make_generic_error("Can't set LMDB number of databases in the environment, error = {}", mma_mdb_strerror(rc));
            }

            if (int rc = mma_mdb_env_set_maxreaders(mdb_env_, 1024)) {
                mma_mdb_env_close(mdb_env_);
                return make_generic_error("Can't set LMDB maximum number of readers in the environment to 1024, error = {}", mma_mdb_strerror(rc));
            }

            int32_t is_readonly = read_only ? MDB_RDONLY : 0;
            if (int rc = mma_mdb_env_open(mdb_env_, file_name_.data(), MDB_NOSUBDIR | MDB_NOTLS | is_readonly, 0664)) {
                mma_mdb_env_close(mdb_env_);
                return make_generic_error("Can't open LMDB database {}, error = {}", file_name_, mma_mdb_strerror(rc));
            }

            MDB_txn* transaction{};

            if (const int rc = mma_mdb_txn_begin(mdb_env_, nullptr, MDB_RDONLY, &transaction)) {
                mma_mdb_env_close(mdb_env_);
                return make_generic_error("Can't start read-only transaction, error = {}", mma_mdb_strerror(rc));
            }

            if (const int rc = mma_mdb_dbi_open(transaction, "memoria_system", 0, &system_db_)) {
                mma_mdb_txn_abort(transaction);
                mma_mdb_env_close(mdb_env_);
                return make_generic_error("Can't open memoria_system database, error = {}", mma_mdb_strerror(rc));
            }

            if (const int rc = mma_mdb_dbi_open(transaction, "memoria_data", 0, &data_db_)) {
                mma_mdb_dbi_close(mdb_env_, system_db_);
                mma_mdb_txn_abort(transaction);
                mma_mdb_env_close(mdb_env_);
                return make_generic_error("Can't open memoria_data database, error = {}", mma_mdb_strerror(rc));
            }

            MDB_val key = {sizeof(DirectoryCtrID), (void*)&DirectoryCtrID};
            MDB_val val{};

            if (const int rc = mma_mdb_get(transaction, system_db_, &key, &val)) {
                mma_mdb_dbi_close(mdb_env_, system_db_);
                mma_mdb_dbi_close(mdb_env_, data_db_);
                mma_mdb_txn_abort(transaction);
                mma_mdb_env_close(mdb_env_);
                return make_generic_error("Can't read superblock from the memoria_system database, error = {}", mma_mdb_strerror(rc));
            }

            if (val.mv_size != BASIC_BLOCK_SIZE) {
                mma_mdb_dbi_close(mdb_env_, system_db_);
                mma_mdb_dbi_close(mdb_env_, data_db_);
                mma_mdb_txn_abort(transaction);
                mma_mdb_env_close(mdb_env_);
                return make_generic_error("Superblock size doesn't match, expected = {}, actual = {}", BASIC_BLOCK_SIZE, val.mv_size);
            }

            superblock_ = ptr_cast<Superblock>(::malloc(BASIC_BLOCK_SIZE));
            std::memcpy(superblock_, val.mv_data, BASIC_BLOCK_SIZE);

            if (const int rc = mma_mdb_txn_commit(transaction)) {
                mma_mdb_dbi_close(mdb_env_, system_db_);
                mma_mdb_dbi_close(mdb_env_, data_db_);
                mma_mdb_txn_abort(transaction);
                mma_mdb_env_close(mdb_env_);
                return make_generic_error("Can't commit reead-only transaction, error = {}", mma_mdb_strerror(rc));
            }

            return VoidResult::of();
        });
    }

    virtual ~LMDBStore() noexcept {
        mma_mdb_env_close(mdb_env_);
        ::free(superblock_);
    }

    virtual VoidResult set_async(bool async) noexcept
    {
        std::lock_guard<std::recursive_mutex> lock(store_mutex_);
        if (const int rc = mma_mdb_env_set_flags(mdb_env_, MDB_NOSYNC, async ? 1 : 0)) {
            return make_generic_error("Can't set asynchronous mode ({}), error = {}", async, mma_mdb_strerror(rc));
        }
        return VoidResult::of();
    }

    virtual VoidResult copy_to(U8String path, bool with_compaction) noexcept
    {
        if (const int rc = mma_mdb_env_copy2(mdb_env_, path.data(), with_compaction ? MDB_CP_COMPACT : 0)) {
            return make_generic_error("Can't copy database to '{}', error = {}", path, mma_mdb_strerror(rc));
        }
        return VoidResult::of();
    }

    virtual VoidResult flush(bool force) noexcept {
        if (const int rc = mma_mdb_env_sync(mdb_env_, force)) {
            return make_generic_error("Can't csync the environment error = {}", mma_mdb_strerror(rc));
        }
        return VoidResult::of();
    }

    virtual Result<ReadOnlyCommitPtr> open() noexcept
    {
        using ResultT = Result<ReadOnlyCommitPtr>;
        MaybeError maybe_error{};
        ReadOnlyCommitPtr ptr{};

        {            
            ptr = snp_make_shared<LMDBStoreReadOnlyCommit<Profile>>(
                maybe_error, this->shared_from_this(), mdb_env_, system_db_, data_db_
            );
        }

        if (!maybe_error) {
            return ResultT::of(std::move(ptr));
        }
        else {
            return std::move(maybe_error.get());
        }
    }

    virtual VoidResult flush() noexcept {
        return VoidResult::of();
    }

    virtual Result<WritableCommitPtr> begin() noexcept
    {
        using ResultT = Result<SnpSharedPtr<LMDBStoreWritableCommit<Profile>>>;

        ResultT res = wrap_throwing([&]() -> ResultT {
            MaybeError maybe_error{};
            auto ptr = snp_make_shared<LMDBStoreWritableCommit<Profile>>(
                maybe_error, this->shared_from_this(), mdb_env_, superblock_, system_db_, data_db_
            );

            if (!maybe_error) {
                return ResultT::of(std::move(ptr));
            }
            else {
                return std::move(maybe_error.get());
            }
        });

        if (res.is_error())
        {
            return MEMORIA_PROPAGATE_ERROR(res);
        }
        else {
            MEMORIA_TRY_VOID(res.get()->finish_commit_opening());
            return Result<WritableCommitPtr>::of(std::move(res).get());
        }
    }

    virtual VoidResult close() noexcept {
        return make_generic_error("Explicitly closing LMDB store is not supported");
    }

    static void init_profile_metadata() noexcept {
        LMDBStoreWritableCommit<Profile>::init_profile_metadata();
    }


private:
    VoidResult init_store()
    {
        MaybeError maybe_error;

        auto ptr = snp_make_shared<LMDBStoreWritableCommit<Profile>>(
            maybe_error, this->shared_from_this(), mdb_env_, superblock_, system_db_, data_db_, InitLMDBStoreTag{}
        );

        if (!maybe_error)
        {
            return ptr->finish_store_initialization();
        }
        else {
            return std::move(maybe_error.get());
        }

        return VoidResult::of();
    }

    virtual Result<Optional<SequenceID>> check(StoreCheckCallbackFn callback) noexcept
    {
        return wrap_throwing([&]() {
            return do_check(callback);
        });
    }


    Result<Optional<SequenceID>> do_check(StoreCheckCallbackFn callback)
    {        
        using ResultT = Result<Optional<SequenceID>>;
        return ResultT::of();
    }
};

}
