
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

#include <memoria/api/store/swmr_store_api.hpp>
#include <memoria/python/generic/python_store_snapshot_generic.hpp>

namespace memoria {

template <typename Profile>
struct PythonAPIBinder<IBasicSWMRStore<Profile>> {
    using Type = IBasicSWMRStore<Profile>;

    using RWCommitType   = ISWMRStoreWritableCommit<Profile>;
    using ROCommitType   = ISWMRStoreReadOnlyCommit<Profile>;
    using CommitBaseType = ISWMRStoreCommitBase<Profile>;

    using SnpCtrOpsType  = IStoreSnapshotCtrOps<Profile>;
    using WritableSnpCtrOpsType = IStoreWritableSnapshotCtrOps<Profile>;
    using CtrReferenceableType = CtrReferenceable<Profile>;

    using CtrID = ApiProfileCtrID<Profile>;

    using SequenceID = uint64_t;

    static void make_bindings(pybind11::module_& m) {
        namespace py = pybind11;

        pybind11::class_<CommitBaseType, SnpCtrOpsType, SharedPtr<CommitBaseType>> commit_base(m, "SWMRStoreCommitBase");
        commit_base.def("commit_id", &CommitBaseType::commit_id);
        commit_base.def("describe_to_cout", &CommitBaseType::describe_to_cout);

        pybind11::class_<ROCommitType, CommitBaseType, SharedPtr<ROCommitType>>(m, "SWMRStoreReanOnlyCommit")
            .def("drop", &ROCommitType::drop);

        pybind11::class_<RWCommitType, CommitBaseType, WritableSnpCtrOpsType, SharedPtr<RWCommitType>>(m, "SWMRStoreWritableCommit")
            .def("set_persistent", &RWCommitType::set_persistent)
            .def("is_persistent", &RWCommitType::is_persistent);

        py::class_<Type, SharedPtr<Type>>(m, "BasicSWMRStore")
            .def("flush", &Type::flush)
            .def("begin", &Type::begin)
            .def("open", &Type::open)
            ;
    }
};

template <typename Profile>
struct PythonAPIBinder<ISWMRStore<Profile>>: PythonAPIBinder<IBasicSWMRStore<Profile>> {
    using Base = PythonAPIBinder<IBasicSWMRStore<Profile>>;
    using Type = ISWMRStore<Profile>;

    using RWCommitType   = ISWMRStoreWritableCommit<Profile>;
    using ROCommitType   = ISWMRStoreReadOnlyCommit<Profile>;
    using CommitBaseType = ISWMRStoreCommitBase<Profile>;

    using SnpCtrOpsType         = IStoreSnapshotCtrOps<Profile>;
    using WritableSnpCtrOpsType = IStoreWritableSnapshotCtrOps<Profile>;
    using CtrReferenceableType  = CtrReferenceable<Profile>;

    using CtrID = ApiProfileCtrID<Profile>;

    using CommitID = ApiProfileSnapshotID<Profile>;
    using typename Base::SequenceID;

    static void make_bindings(pybind11::module_& m) {
        namespace py = pybind11;

        py::class_<Type, IBasicSWMRStore<Profile>, SharedPtr<Type>>(m, "SWMRStore")
            .def("open", py::overload_cast<const CommitID&, bool>(&Type::open))
            .def("open", py::overload_cast<>(&Type::open));

        m.def("create_swmr_store", &create_swmr_store);
        m.def("open_swmr_store", &open_swmr_store);

        m.def("create_lite_swmr_store", &create_lite_swmr_store);
        m.def("open_lite_swmr_store", &open_lite_swmr_store);
    }
};


template <typename Profile>
struct PythonAPIBinder<ILMDBStore<Profile>>: PythonAPIBinder<IBasicSWMRStore<Profile>> {
    using Base = PythonAPIBinder<IBasicSWMRStore<Profile>>;
    using Type = ILMDBStore<Profile>;

    using RWCommitType   = ISWMRStoreWritableCommit<Profile>;
    using ROCommitType   = ISWMRStoreReadOnlyCommit<Profile>;
    using CommitBaseType = ISWMRStoreCommitBase<Profile>;

    using SnpCtrOpsType         = IStoreSnapshotCtrOps<Profile>;
    using WritableSnpCtrOpsType = IStoreWritableSnapshotCtrOps<Profile>;
    using CtrReferenceableType  = CtrReferenceable<Profile>;

    using CtrID = ApiProfileCtrID<Profile>;

    using CommitID = int64_t;
    using typename Base::SequenceID;

    static void make_bindings(pybind11::module_& m) {
        namespace py = pybind11;

        py::class_<Type, IBasicSWMRStore<Profile>, SharedPtr<Type>> lmdb_cls(m, "LMDBStore");

        lmdb_cls.def("copy_to", &Type::copy_to);
        lmdb_cls.def("set_async", &Type::set_async);
        lmdb_cls.def("flush", &Type::flush);

        m.def("create_lmdb_store", &create_lmdb_store);
        m.def("open_lmdb_store", &open_lmdb_store);
        m.def("open_lmdb_store_readonly", &open_lmdb_store_readonly);
    }
};


}
