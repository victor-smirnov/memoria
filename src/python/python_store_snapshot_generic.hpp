
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

#include <memoria/core/container/ctr_referenceable.hpp>

#include <memoria/api/store/store_api_common.hpp>

#include "python_commons.hpp"

namespace memoria {

template <typename T>
class StoreSnapshotOps {};

template <typename Profile>
struct PythonAPIBinder<StoreSnapshotOps<Profile>> {

    using SnpCtrOpsType = IStoreSnapshotCtrOps<Profile>;
    using WritableSnpCtrOpsType = IStoreWritableSnapshotCtrOps<Profile>;
    using CtrReferenceableType = CtrReferenceable<Profile>;

    using CtrID = ProfileCtrID<Profile>;

    using CommitID = int64_t;
    using SequenceID = uint64_t;

    static void make_bindings(pybind11::module_& m) {
        namespace py = pybind11;

        py::class_<SnpCtrOpsType, SnpSharedPtr<SnpCtrOpsType>> snp_ctr_ops(m, "SnpCtrOps");

        snp_ctr_ops.def("is_committed", &SnpCtrOpsType::is_committed);
        snp_ctr_ops.def("is_active", &SnpCtrOpsType::is_active);
        snp_ctr_ops.def("is_marked_to_clear", &SnpCtrOpsType::is_marked_to_clear);
        snp_ctr_ops.def("dump_open_containers", &SnpCtrOpsType::dump_open_containers);
        snp_ctr_ops.def("has_open_containers", &SnpCtrOpsType::has_open_containers);
        snp_ctr_ops.def("container_names", &SnpCtrOpsType::container_names);
        snp_ctr_ops.def("drop", &SnpCtrOpsType::drop);
        snp_ctr_ops.def("check", &SnpCtrOpsType::check);
        snp_ctr_ops.def("ctr_type_name_for", &SnpCtrOpsType::ctr_type_name_for);
        snp_ctr_ops.def("find", &SnpCtrOpsType::find);

        py::class_<
                WritableSnpCtrOpsType,
                SnpSharedPtr<WritableSnpCtrOpsType>
        > writable_snp_ctr_ops(m, "WritableSnpCtrOps", snp_ctr_ops);

        writable_snp_ctr_ops.def("flush_open_containers", &WritableSnpCtrOpsType::flush_open_containers);
        writable_snp_ctr_ops.def("commit", &WritableSnpCtrOpsType::commit);
        writable_snp_ctr_ops.def("drop_ctr", &WritableSnpCtrOpsType::drop_ctr);
        writable_snp_ctr_ops.def("clone_ctr2", py::overload_cast<const CtrID&, const CtrID&>(&WritableSnpCtrOpsType::clone_ctr));
        writable_snp_ctr_ops.def("clone_ctr", py::overload_cast<const CtrID&>(&WritableSnpCtrOpsType::clone_ctr));
        writable_snp_ctr_ops.def("create2", py::overload_cast<U8StringView, const CtrID&>(&WritableSnpCtrOpsType::create));
        writable_snp_ctr_ops.def("create", py::overload_cast<U8StringView>(&WritableSnpCtrOpsType::create));
    }
};

}
