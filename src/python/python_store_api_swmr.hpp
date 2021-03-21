
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

#include "python_store_snapshot_generic.hpp"

namespace memoria {

template <typename Profile>
struct PythonAPIBinder<ISWMRStore<Profile>> {
    using Type = ISWMRStore<Profile>;

    using RWCommitType   = ISWMRStoreWritableCommit<Profile>;
    using ROCommitType   = ISWMRStoreReadOnlyCommit<Profile>;
    using CommitBaseType = ISWMRStoreCommitBase<Profile>;

    using SnpCtrOpsType = IStoreSnapshotCtrOps<Profile>;
    using WritableSnpCtrOpsType = IStoreWritableSnapshotCtrOps<Profile>;
    using CtrReferenceableType = CtrReferenceable<Profile>;

    using CtrID = ProfileCtrID<Profile>;

    using CommitID = int64_t;
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

        py::class_<Type, SharedPtr<Type>>(m, "SWMRStore")
            .def("close", &Type::close)
            .def("flush", &Type::flush)
            .def("begin", &Type::begin)
            .def("open", py::overload_cast<CommitID>(&Type::open))
            .def("open_latest", py::overload_cast<>(&Type::open))
            ;

        m.def("create_mapped_swmr_store", &create_mapped_swmr_store);
        m.def("open_mapped_swmr_store", &open_mapped_swmr_store);
    }
};

}
