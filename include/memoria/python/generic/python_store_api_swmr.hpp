
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

template <>
struct PythonAPIBinder<SWMRParams> {
    using Type = SWMRParams;

    static void make_bindings(pybind11::module_& m) {
        namespace py = pybind11;

        pybind11::class_<Type, SharedPtr<Type>> clazz(m, "SWMRParams");

        clazz.def(py::init<uint64_t>());
        clazz.def(py::init<>());
        clazz.def("open_readonly", &Type::open_read_only);
        clazz.def("file_size", &Type::file_size);
        clazz.def("is_read_only", &Type::is_read_only);
    }
};


template <typename Profile>
struct PythonAPIBinder<ISWMRStoreHistoryView<Profile>> {
    using Type = ISWMRStoreHistoryView<Profile>;

    using CtrID     = ApiProfileCtrID<Profile>;
    using SnapshotID  = ApiProfileSnapshotID<Profile>;

    using SequenceID = uint64_t;

    static void make_bindings(pybind11::module_& m) {
        namespace py = pybind11;

        pybind11::class_<Type, SharedPtr<Type>> clazz(m, "SWMRStoreHistoryView");

        clazz.def("is_transient", &Type::is_transient);
        clazz.def("is_system_snapshot", &Type::is_system_snapshot);
        clazz.def("parent", &Type::parent);
        clazz.def("children", &Type::children);
        clazz.def("snapshots", &Type::snapshots);
        clazz.def("branch_head", &Type::branch_head);
        clazz.def("branch_names", &Type::branch_names);
    }
};


template <typename Profile>
struct PythonAPIBinder<IBasicSWMRStore<Profile>> {
    using Type = IBasicSWMRStore<Profile>;

    using RWSnapshotType   = ISWMRStoreWritableSnapshot<Profile>;
    using ROSnapshotType   = ISWMRStoreReadOnlySnapshot<Profile>;
    using SnapshotBaseType = ISWMRStoreSnapshotBase<Profile>;

    using SnpCtrOpsType  = IROStoreSnapshotCtrOps<Profile>;
    using WritableSnpCtrOpsType = IROStoreWritableSnapshotCtrOps<Profile>;
    using CtrReferenceableType = CtrReferenceable<Profile>;

    using CtrID = ApiProfileCtrID<Profile>;

    using SequenceID = uint64_t;

    static void make_bindings(pybind11::module_& m) {
        namespace py = pybind11;

        pybind11::class_<SnapshotBaseType, SnpCtrOpsType, SharedPtr<SnapshotBaseType>> snapshot_base(m, "SWMRStoreSnapshotBase");
        snapshot_base.def("snapshot_id", &SnapshotBaseType::snapshot_id);
        snapshot_base.def("describe_to_cout", &SnapshotBaseType::describe_to_cout);
        snapshot_base.def("is_system_snapshot", &ROSnapshotType::is_system_snapshot);
        snapshot_base.def("is_persistent", &ROSnapshotType::is_transient);

        pybind11::class_<ROSnapshotType, SnapshotBaseType, SharedPtr<ROSnapshotType>>(m, "SWMRStoreReanOnlySnapshot")
            .def("drop", &ROSnapshotType::drop)
            ;

        pybind11::class_<RWSnapshotType, SnapshotBaseType, WritableSnpCtrOpsType, SharedPtr<RWSnapshotType>>(m, "SWMRStoreWritableSnapshot")
            .def("set_transient", &RWSnapshotType::set_transient)
            .def("prepare", &RWSnapshotType::prepare)
            .def("rollback", &RWSnapshotType::rollback)
            .def("remove_branch", &RWSnapshotType::remove_branch)
            .def("remove_snapshot", &RWSnapshotType::remove_snapshot)
            ;

        py::class_<Type, SharedPtr<Type>>(m, "BasicSWMRStore")
            .def("begin", &Type::begin)
            .def("open", &Type::open)
            ;
    }
};

template <typename Profile>
struct PythonAPIBinder<ISWMRStore<Profile>>: PythonAPIBinder<IBasicSWMRStore<Profile>> {
    using Base = PythonAPIBinder<IBasicSWMRStore<Profile>>;
    using Type = ISWMRStore<Profile>;

    using RWSnapshotType   = ISWMRStoreWritableSnapshot<Profile>;
    using ROSnapshotType   = ISWMRStoreReadOnlySnapshot<Profile>;
    using SnapshotBaseType = ISWMRStoreSnapshotBase<Profile>;

    using SnpCtrOpsType         = IROStoreSnapshotCtrOps<Profile>;
    using WritableSnpCtrOpsType = IROStoreWritableSnapshotCtrOps<Profile>;
    using CtrReferenceableType  = CtrReferenceable<Profile>;

    using CtrID = ApiProfileCtrID<Profile>;

    using SnapshotID = ApiProfileSnapshotID<Profile>;
    using typename Base::SequenceID;

    static void make_bindings(pybind11::module_& m) {
        namespace py = pybind11;

        py::enum_<FlushType> ft(m, "FlushType");
        ft.value("DEFAULT", FlushType::DEFAULT);
        ft.value("FULL", FlushType::FULL);
        ft.export_values();

        py::enum_<ConsistencyPoint> cp(m, "ConsistencyPoint");
        cp.value("YES", ConsistencyPoint::YES);
        cp.value("NO", ConsistencyPoint::NO);
        cp.value("AUTO", ConsistencyPoint::AUTO);
        cp.value("FULL", ConsistencyPoint::FULL);
        cp.export_values();

        py::class_<Type, IBasicSWMRStore<Profile>, SharedPtr<Type>>(m, "SWMRStore")
            .def("open", py::overload_cast<const SnapshotID&, bool>(&Type::open))
            .def("open", py::overload_cast<U8StringView>(&Type::open))
            .def("history_view", &Type::history_view)
            .def("is_transient", &Type::is_transient)
            .def("is_system_snapshot", &Type::is_system_snapshot)
            .def("parent", &Type::parent)
            .def("children", &Type::children)
            .def("snapshots", &Type::snapshots)
            .def("branch_names", &Type::branches)
            .def("close", &Type::close)
            .def("flush", &Type::flush, py::arg("ft") = FlushType::DEFAULT)
            .def("begin", py::overload_cast<U8StringView>(&Type::begin))
            .def("branch_from", py::overload_cast<const SnapshotID&, U8StringView>(&Type::branch_from))
            .def("branch_from", py::overload_cast<U8StringView, U8StringView>(&Type::branch_from))
            .def("count_volatile_snapshots", &Type::count_volatile_snapshots)
            ;

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

    using RWSnapshotType   = ISWMRStoreWritableSnapshot<Profile>;
    using ROSnapshotType   = ISWMRStoreReadOnlySnapshot<Profile>;
    using SnapshotBaseType = ISWMRStoreSnapshotBase<Profile>;

    using SnpCtrOpsType         = IROStoreSnapshotCtrOps<Profile>;
    using WritableSnpCtrOpsType = IROStoreWritableSnapshotCtrOps<Profile>;
    using CtrReferenceableType  = CtrReferenceable<Profile>;

    using CtrID = ApiProfileCtrID<Profile>;

    using SnapshotID = int64_t;
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
