
// Copyright 2021-2022 Victor Smirnov
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

#include <memoria/core/types.hpp>

#include <memoria/api/store/store_api_common.hpp>

#include <memoria/api/set/set_api.hpp>
#include <memoria/core/datatypes/datatypes.hpp>
#include <memoria/core/memory/memory.hpp>
#include <memoria/core/tools/random.hpp>
#include <memoria/core/tools/ticker.hpp>

#include <memoria/api/store/swmr_store_api.hpp>
#include <memoria/api/store/memory_store_api.hpp>

namespace memoria {

template <typename StoreT>
struct StoreOperations {

    using ProfileT = typename UnPtr<StoreT>::ProfileT;
    using ReadOnlySnapshotPtr = typename UnPtr<StoreT>::ReadOnlySnapshotPtr;
    using WritableSnapshotPtr = typename UnPtr<StoreT>::WritableSnapshotPtr;

    using CtrID = ApiProfileCtrID<ProfileT>;

    virtual ~StoreOperations() = default;

    virtual CtrID ctr_id() = 0;

    virtual StoreT open_store() = 0;
    virtual StoreT create_store() = 0;

    virtual void close_store(StoreT store) = 0;
    virtual bool is_store_closed(StoreT store) = 0;
    virtual void commit(WritableSnapshotPtr snp, ConsistencyPoint cp) = 0;

    virtual void flush(StoreT store) = 0;

    virtual WritableSnapshotPtr begin_writable(StoreT store) = 0;
    virtual ReadOnlySnapshotPtr open_read_only(StoreT store) = 0;
};


template <typename StoreT, typename ProfileT = typename UnPtr<StoreT>::ProfileT>
class StoreTestBench {
    using OpsT = StoreOperations<StoreT>;
    using OpsTPtr = std::shared_ptr<OpsT>;

    using CtrType = Set<Varchar>;
    using CtrID = ApiProfileCtrID<ProfileT>;
    using CtrSizeT = ApiProfileCtrSizeT<ProfileT>;

    std::shared_ptr<OpsT> store_ops_;

    CtrID ctr_id_;

    CtrSizeT entries_{100000};
    CtrSizeT batch_size_{1000};
    CtrSizeT timing_epocs_{10};

    ConsistencyPoint consistency_point_{ConsistencyPoint::AUTO};
    bool check_epocs_{true};
    bool check_store_{true};
    bool read_back_{true};

    std::vector<U8String> data_;

public:
    StoreTestBench(OpsTPtr store_ops): store_ops_(store_ops)
    {
        ctr_id_ = store_ops_->ctr_id();
    }

    void set_entries(CtrSizeT v) {
        entries_ = v;
    }

    void set_batch_size(CtrSizeT v) {
        batch_size_ = v;
    }

    void set_timing_epocs(CtrSizeT v) {
        timing_epocs_ = v;
    }

    void set_consistency_point(ConsistencyPoint cp) {
        consistency_point_ = cp;
    }

    void set_check_epocs(bool vv) {
        check_epocs_ = vv;
    }

    void set_check_store(bool vv) {
        check_store_ = vv;
    }

    void set_read_back(bool vv) {
        read_back_ = vv;
    }

    virtual void run_insertions()
    {
        StoreT store = store_ops_->create_store();

        CheckResultConsumerFn callback = [](CheckSeverity svr, const ld::LDDocument& doc){
            std::cout << doc.to_string() << std::endl;
            if (svr == CheckSeverity::ERROR) {
                MEMORIA_MAKE_GENERIC_ERROR("Container check error").do_throw();
            }
        };

        {
            auto snp = store_ops_->begin_writable(store);
            auto ctr = create(snp, CtrType(), ctr_id_);
            store_ops_->commit(snp, consistency_point_);

            if (check_epocs_) {
                store->check(callback);
            }
        }

        Ticker ticker(entries_ / timing_epocs_);

        {
            CtrSizeT epoc_commits{};

            for (CtrSizeT entry = 0; entry < entries_;)
            {
                auto snp = store_ops_->begin_writable(store);
                auto ctr = find<CtrType>(snp, ctr_id_);

                for (CtrSizeT bb = 0; bb < batch_size_; bb++, entry++)
                {
                    U8String str = format_u8("Cool String ABCDEFGH :: {}", getBIRandomG());
                    ctr->upsert(str);

                    if (read_back_ && !ctr->contains(str)) {
                        MEMORIA_MAKE_GENERIC_ERROR("Can't find key {} in the container", str).do_throw();
                    }

                    data_.push_back(str);

                    ticker.tick();
                }

                store_ops_->commit(snp, consistency_point_);
                epoc_commits++;

                if (ticker.is_threshold())
                {
                    println("Processed {} entries of {}, {} commits in {} ms", entry, entries_, epoc_commits, ticker.duration());
                    epoc_commits = 0;
                    ticker.next();

                    if (check_epocs_) {
                        store->check(callback);
                    }
                }
            }
        }

        store_ops_->flush(store);

        println("Total insertion time: {}", FormatTime(getTimeInMillis() - ticker.start_time()));

        if (check_store_) {
            store->check(callback);
        }

        store_ops_->close_store(store);

        std::sort(data_.begin(), data_.end());
    }

    virtual void run_queries()
    {
        StoreT store = store_ops_->open_store();
        auto snp = store_ops_->open_read_only(store);
        auto ctr = find<CtrType>(snp, ctr_id_);

        int64_t t_start = getTimeInMillis();
        CtrSizeT cnt{};
        auto ii = data_.begin();
        ctr->for_each([&](auto key){
            if (ii != data_.end()) {
                if (U8String(key) == *ii) {
                    ++cnt;
                }
                ++ii;
            }
        });

        if (cnt != ctr->size()) {
            MEMORIA_MAKE_GENERIC_ERROR("Iteration/size mismatch. Expected: {}, actual: {}", ctr->size(), cnt).do_throw();
        }

        println("Container read time: {}", FormatTime(getTimeInMillis() - t_start));
    }
};



template <typename Store = ISWMRStore<CoreApiProfile>>
class AbstractSWMRStoreOperation: public StoreOperations<AllocSharedPtr<Store>> {
protected:
    using Base = memoria::StoreOperations<AllocSharedPtr<Store>>;

    using typename Base::CtrID;
    using typename Base::WritableSnapshotPtr;
    using typename Base::ReadOnlySnapshotPtr;
    using typename Base::ProfileT;

    using StoreT = SnpSharedPtr<Store>;

    U8String file_name_;

    uint64_t store_size_{1024};
    bool remove_existing_{true};

public:
    AbstractSWMRStoreOperation(U8String file_name, uint64_t store_size):
        file_name_(file_name), store_size_(store_size)
    {}

    virtual CtrID ctr_id() {
        return CtrID::make_random();
    }

    virtual bool is_store_closed(StoreT store) {
        return false;
    }

    virtual void flush(StoreT store) {
        store->flush();
    }

    virtual void commit(WritableSnapshotPtr snp, ConsistencyPoint cp) {
        snp->commit();
    }

    virtual WritableSnapshotPtr begin_writable(StoreT store) {
        return store->begin();
    }

    virtual ReadOnlySnapshotPtr open_read_only(StoreT store) {
        return store->open();
    }
};


class LiteSWMRStoreOperation: public AbstractSWMRStoreOperation<> {
protected:
    using Base = AbstractSWMRStoreOperation<>;
    using typename Base::StoreT;


public:
    LiteSWMRStoreOperation(U8String file_name, uint64_t store_size):
        Base(file_name, store_size)
    {}


    virtual StoreT open_store() {
        return open_lite_swmr_store(file_name_);
    }

    virtual StoreT create_store()
    {
        if (remove_existing_) {
            filesystem::remove(file_name_.data());
        }

        return create_lite_swmr_store(file_name_, store_size_);
    }

    virtual void close_store(StoreT store) {
        store->close();
    }
};


class SWMRStoreOperation: public AbstractSWMRStoreOperation<> {
protected:
    using Base = AbstractSWMRStoreOperation<>;
    using typename Base::StoreT;


public:
    SWMRStoreOperation(U8String file_name, uint64_t store_size):
        Base(file_name, store_size)
    {}


    virtual StoreT open_store() {
        return open_swmr_store(file_name_);
    }

    virtual StoreT create_store()
    {
        if (remove_existing_) {
            filesystem::remove(file_name_.data());
        }

        return create_swmr_store(file_name_, store_size_);
    }

    virtual void close_store(StoreT store) {
        store->close();
    }
};


class LMDBStoreOperation: public AbstractSWMRStoreOperation<ILMDBStore<CoreApiProfile>> {
protected:
    using Base = AbstractSWMRStoreOperation<ILMDBStore<CoreApiProfile>>;
    using typename Base::StoreT;


public:
    LMDBStoreOperation(U8String file_name, uint64_t store_size):
        Base(file_name, store_size)
    {}


    virtual StoreT open_store() {
        return open_lmdb_store(file_name_);
    }

    virtual StoreT create_store()
    {
        if (remove_existing_) {
            filesystem::remove(file_name_.data());
        }

        return create_lmdb_store(file_name_, store_size_);
    }

    virtual void close_store(StoreT store) {
        // not supported
        //store->close();
    }
};

class MemoryStoreOperation: public StoreOperations<AllocSharedPtr<IMemoryStore<CoreApiProfile>>> {
protected:
    using Base = StoreOperations<AllocSharedPtr<IMemoryStore<CoreApiProfile>>>;
    using StoreT = AllocSharedPtr<IMemoryStore<CoreApiProfile>>;
    using typename Base::ProfileT;
    using typename Base::CtrID;

    CtrID ctr_id_;
    U8String file_name_;

public:
    MemoryStoreOperation(U8String file_name):
        file_name_(file_name)
    {
        ctr_id_ = CtrID::make_random();
    }


    virtual CtrID ctr_id() {
        return ctr_id_;
    }

    virtual StoreT open_store() {
        return load_memory_store(file_name_);
    }

    virtual StoreT create_store() {
        return create_memory_store();
    }

    virtual void close_store(StoreT store) {
        store->store(file_name_);
    }

    virtual bool is_store_closed(StoreT store) {
        return true;
    }

    virtual void flush(StoreT store) {
        store->store(file_name_);
    }

    virtual void commit(WritableSnapshotPtr snp, ConsistencyPoint cp) {
        snp->commit(cp);
        snp->set_as_master();
    }

    virtual WritableSnapshotPtr begin_writable(StoreT store) {
        auto snp = store->master()->branch();
        return snp;
    }

    virtual ReadOnlySnapshotPtr open_read_only(StoreT store) {
        return store->master();
    }
};


}
