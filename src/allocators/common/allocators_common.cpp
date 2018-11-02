
// Copyright 2018 Victor Smirnov
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


#include <memoria/v1/api/allocator/allocator_inmem_api_common.hpp>

namespace memoria {
namespace v1 {


void print(std::ostream& out, const ContainerMemoryStat& stat)
{
    out << "ContainerStat{" << stat.ctr_type_name() << "::" << stat.ctr_name() << " Data: ";
    out << stat.total_size() << ", " << stat.total_leaf_size() << ", " << stat.total_branch_size() << " Pages: ";
    out << stat.total_leaf_pages() << ", " << stat.total_branch_pages() << "}";
}

namespace {
    void print_tabs(std::ostream& out, int ntabs) {
        for (int n = 0; n < ntabs; n++) {
            out << "\t";
        }
    }
}

void print(std::ostream& out, const SnapshotMemoryStat& stat, int ntabs)
{
    print_tabs(out, ntabs);

    out << "SnapshotStat[" << stat.snapshot_id();

    if (stat.containers().size() > 0)
    {
        out << "\n";
        print_tabs(out, ntabs + 1);
        out << "Mem: " << stat.total_size() << ", " << stat.total_data_size() << ", " << stat.total_ptree_size() << "\n";

        for (auto ctr_stat: stat.containers())
        {
            print_tabs(out, ntabs + 1);
            print(out, *(ctr_stat.second.get()));
            out << "\n";
        }
    }
    else {
        out << ", Mem: " << stat.total_size() << ", " << stat.total_data_size() << ", " << stat.total_ptree_size();
    }

    print_tabs(out, ntabs);
    out << "]";
}

void print(std::ostream& out, const AllocatorMemoryStat& stat)
{
    out << "AllocatorStat[Mem: " << stat.total_size();

    if (stat.snapshots().size() > 0)
    {
        out << "\n";
        for (auto snp: stat.snapshots())
        {
            print(out, *snp.second.get(), 1);
            out << "\n";
        }

        out << "\t";
    }
    else {}

    out << "]";
}

}}
