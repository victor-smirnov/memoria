
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
#include <memoria/core/strings/string_buffer.hpp>

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
    using ApiBlockID = ApiProfileBlockID<ApiProfileT>;

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

    friend SharedPtr<ILMDBStore<ApiProfileT>> open_lmdb_store(U8StringView);
    friend SharedPtr<ILMDBStore<ApiProfileT>> open_lmdb_store_readonly(U8StringView);
    friend SharedPtr<ILMDBStore<ApiProfileT>> create_lmdb_store(U8StringView, uint64_t);

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

    virtual void set_async(bool async)
    {
        std::lock_guard<std::recursive_mutex> lock(store_mutex_);
        if (const int rc = mma_mdb_env_set_flags(mdb_env_, MDB_NOSYNC, async ? 1 : 0)) {
            make_generic_error("Can't set asynchronous mode ({}), error = {}", async, mma_mdb_strerror(rc)).do_throw();
        }        
    }

    virtual void copy_to(U8String path, bool with_compaction)
    {
        if (const int rc = mma_mdb_env_copy2(mdb_env_, path.data(), with_compaction ? MDB_CP_COMPACT : 0)) {
            return make_generic_error("Can't copy database to '{}', error = {}", path, mma_mdb_strerror(rc)).do_throw();
        }
    }

    virtual void flush(bool force) {
        if (const int rc = mma_mdb_env_sync(mdb_env_, force)) {
            return make_generic_error("Can't sync the environment error = {}", mma_mdb_strerror(rc)).do_throw();
        }
    }

    virtual ReadOnlyCommitPtr open()
    {

        MaybeError maybe_error{};
        auto ptr = snp_make_shared<LMDBStoreReadOnlyCommit<Profile>>(
                maybe_error, this->shared_from_this(), mdb_env_, system_db_, data_db_
        );


        if (!maybe_error) {
            ptr->post_init().throw_if_error();
            return std::move(ptr);
        }
        else {
            std::move(maybe_error.get()).do_throw();
        }
    }

    virtual ReadOnlyCommitPtr flush() {
        return ReadOnlyCommitPtr{};
    }

    virtual WritableCommitPtr begin()
    {
        MaybeError maybe_error{};
        auto ptr = snp_make_shared<LMDBStoreWritableCommit<Profile>>(
            maybe_error, this->shared_from_this(), mdb_env_, superblock_, system_db_, data_db_
        );

        if (!maybe_error) {
            ptr->post_init(maybe_error);
            if (!maybe_error)
            {
                ptr->finish_commit_opening();
                return std::move(ptr);
            }
        }

        maybe_error.get().do_throw();
    }

    virtual void close() {
        make_generic_error("Explicitly closing LMDB store is not supported").do_throw();
    }

    static void init_profile_metadata() noexcept {
        LMDBStoreWritableCommit<Profile>::init_profile_metadata();
    }

    static bool is_my_file(U8String file_name)
    {
        auto name = file_name.to_std_string();

        if (filesystem::exists(name)){
            uint64_t mdb_block_size = 4096;

            if (filesystem::file_size(name) >= mdb_block_size * 2)
            {
                auto memblock = allocate_system<uint8_t>(mdb_block_size);

                int fd = ::open(name.c_str(), O_RDONLY);
                int rr = ::read(fd, memblock.get(), mdb_block_size);
                ::close(fd);

                if (rr < mdb_block_size) {
                    return false;
                }

                return is_my_block(memblock.get());
            }
        }

        return false;
    }

    static bool is_my_block(const uint8_t* mem_block) noexcept {
        uint32_t mdb_magick = 0xBEEFC0DE;
        const uint32_t* meta = ptr_cast<uint32_t>(mem_block + 16);
        return *meta == mdb_magick;
    }

    virtual U8String describe() const {
        return format_u8("LMDBStore<{}>", TypeNameFactory<Profile>::name());
    }

private:
    void init_store()
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
            std::move(maybe_error.get()).do_throw();
        }
    }

    virtual Optional<SequenceID> check(const CheckResultConsumerFn& consumer)
    {
        return do_check(consumer);
    }


    Optional<SequenceID> do_check(const CheckResultConsumerFn& consumer)
    {
        // FIXME: Implementation!
        return Optional<SequenceID>{};
    }

    virtual U8String to_string(const ApiBlockID& block_id) {
        BlockID id;
        block_id_holder_to(block_id, id);
        return format_u8("{}", id);
    }

};

}
