
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

#include <type_traits>

namespace memoria {
namespace v1 {


namespace {
    void print_tabs(std::ostream& out, int ntabs) {
        for (int n = 0; n < ntabs; n++) {
            out << "\t";
        }
    }

    class ListState {
        int32_t cnt_{};
    public:
        void tick() {cnt_++;}

        operator bool() const {
            return cnt_ > 0;
        }
    };

    template <typename T>
    std::enable_if_t<std::is_arithmetic<T>::value, void> out_json_val(ListState& state, std::ostream& out, const char* name, const T& val)
    {
        if (val != 0)
        {
            if (state) {
                out << ", ";
            }
            out << "\"" << name << "\": " << val;
            state.tick();
        }
    }

    template <typename T>
    std::enable_if_t<!std::is_arithmetic<T>::value, void>  out_json_val(ListState& state, std::ostream& out, const char* name, const T& val)
    {
        if (state) {
            out << ", ";
        }

        out << "\"" << name << "\": \"" << val << "\"";

        state.tick();
    }
}



void print(std::ostream& out, const ContainerMemoryStat& stat)
{
    out << "ContainerStat{" << stat.ctr_type_name() << "::" << stat.ctr_name() << " Data: ";
    out << stat.total_size() << ", " << stat.total_leaf_size() << ", " << stat.total_branch_size() << " Pages: ";
    out << stat.total_leaf_blocks() << ", " << stat.total_branch_blocks() << "}";
}





void print_json(std::ostream& out, const ContainerMemoryStat& stat)
{
    ListState state;

    out << "{";

    out_json_val(state, out, "ctrTypeName", stat.ctr_type_name());
    out_json_val(state, out, "totalSize", stat.total_size());
    out_json_val(state, out, "totalLeafSize", stat.total_leaf_size());
    out_json_val(state, out, "totalBranchSize", stat.total_branch_size());
    out_json_val(state, out, "totalLeafPages", stat.total_leaf_blocks());
    out_json_val(state, out, "totalBranchPages", stat.total_branch_blocks());

    out << "}";
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


void print_json(std::ostream& out, const SnapshotMemoryStat& stat)
{
    ListState state;

    out << "{";

    out_json_val(state, out, "totalSize", stat.total_size());
    out_json_val(state, out, "totalDataSize", stat.total_data_size());
    out_json_val(state, out, "totalPTreeSize", stat.total_ptree_size());

    if (stat.containers().size() > 0)
    {
        if (state) out << ", ";
        out << "\"containers\": {";

        ListState state2;
        for (auto ctr_stat: stat.containers())
        {
            if (state2) out << ", ";

            out << "\"" << ctr_stat.first << "\":";
            print_json(out, *(ctr_stat.second.get()));

            state2.tick();
        }

        out << "}";
    }

    out << "}";
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

void print_json(std::ostream& out, const AllocatorMemoryStat& stat)
{
    ListState state;

    out << "{";
    out_json_val(state, out, "totalSize", stat.total_size());

    if (stat.snapshots().size() > 0)
    {
        if (state) out << ", ";
        out << "\"snapshots\": {";

        ListState state2;
        for (auto snp_stat: stat.snapshots())
        {
            if (state2) out << ", ";

            out << "\"" << snp_stat.first << "\":";
            print_json(out, *(snp_stat.second.get()));

            state2.tick();
        }

        out << "}";
    }
    else {}

    out << "}";
}

}}
