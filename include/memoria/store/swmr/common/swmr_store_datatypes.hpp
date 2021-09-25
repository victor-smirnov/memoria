
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

#include <memoria/profiles/common/common.hpp>
#include <memoria/core/tools/optional.hpp>

#include <memoria/core/types/typehash.hpp>
#include <memoria/core/tools/reflection.hpp>

#include <memoria/core/datatypes/core.hpp>
#include <memoria/core/datatypes/traits.hpp>

#include <memoria/core/strings/format.hpp>

#include <ostream>

namespace memoria {

template <typename Profile>
class SWMRSnapshotMetadata {

    using SnapshotID = ApiProfileSnapshotID<Profile>;
    SnapshotID parent_snapshot_id_;
    uint64_t superblock_file_pos_;
    uint64_t flags_;
    uint64_t timestamp_;
    uint64_t ttl_;

public:
    template<typename> friend struct FieldFactory;

    static constexpr uint64_t VERSION = 1;
    enum Bits: uint64_t {
        NONE = 0x0,
        TRANSIENT = 0x1,
        SYSTEM_SNAPSHOT = 0x2,
        DATA_SNAPSHOT = 0x4,
        HAS_TTL = 0x8,
        REMOVED_BRANCH = 0x10
    };

    SWMRSnapshotMetadata() noexcept :
        superblock_file_pos_(), flags_()
    {}

    const SnapshotID& parent_snapshot_id() const noexcept {
        return parent_snapshot_id_;
    }

    void set_parent_snapshot_id(const SnapshotID& id) noexcept {
        parent_snapshot_id_ = id;
    }

    uint64_t superblock_file_pos() const noexcept {
        return superblock_file_pos_;
    }

    void set_superblock_file_pos(uint64_t pos) noexcept {
        superblock_file_pos_ = pos;
    }



    uint64_t flags() const noexcept {
        return flags_;
    }

    void set_flags(uint64_t ff) noexcept {
        flags_ = ff;
    }

    bool is_transient() const noexcept {
        return flags_ & TRANSIENT;
    }

    bool is_system_snapshot() const noexcept {
        return flags_ & SYSTEM_SNAPSHOT;
    }

    bool is_data_snapshot() const noexcept {
        return flags_ & DATA_SNAPSHOT;
    }

    bool has_ttl() const noexcept {
        return flags_ & HAS_TTL;
    }

    bool is_removed_branch() const noexcept {
        return flags_ & REMOVED_BRANCH;
    }


    void set_transient(bool value) noexcept {
        if (value) {
            flags_ |= TRANSIENT;
        }
        else {
            flags_ &= ~TRANSIENT;
        }
    }

    void set_system_snapshot(bool value) noexcept {
        if (value) {
            flags_ |= SYSTEM_SNAPSHOT;
        }
        else {
            flags_ &= ~SYSTEM_SNAPSHOT;
        }
    }

    void set_data_snapshot(bool value) noexcept {
        if (value) {
            flags_ |= DATA_SNAPSHOT;
        }
        else {
            flags_ &= ~DATA_SNAPSHOT;
        }
    }

    void set_removed_branch(bool value) noexcept {
        if (value) {
            flags_ |= REMOVED_BRANCH;
        }
        else {
            flags_ &= ~REMOVED_BRANCH;
        }
    }

    void set_ttl(uint64_t ttl) noexcept
    {
        flags_ |= HAS_TTL;
        ttl_ = ttl;
    }

    uint64_t ttl() const noexcept {
        return ttl_;
    }




    void set_timestamp(uint64_t value) noexcept {
        timestamp_ = value;
    }

    uint64_t timestamp() const noexcept {
        return timestamp_;
    }


    bool operator==(const SWMRSnapshotMetadata& other) const noexcept {
        return parent_snapshot_id_ == other.parent_snapshot_id_ &&
                superblock_file_pos_ == other.superblock_file_pos_ &&
                flags_ == other.flags_ &&
                timestamp_ == other.timestamp_ &&
                ttl_ = other.ttl_;
    }

    bool operator!=(const SWMRSnapshotMetadata& other) const noexcept {
        return parent_snapshot_id_ != other.parent_snapshot_id_ ||
                superblock_file_pos_ != other.superblock_file_pos_ ||
                flags_ != other.flags_ ||
                timestamp_ != other.timestamp_ ||
                ttl_ != other.ttl_
                ;
    }
};


template <typename Profile>
std::ostream& operator<<(std::ostream& out, const SWMRSnapshotMetadata<Profile>& meta) {

    out << "[";
    out << meta.parent_snapshot_id();
    out << ", ";
    out << meta.superblock_file_pos();
    out << ", ";
    out << meta.flags();
    out << "]";

    return out;
}


template <typename T> struct FieldFactory;

template <typename Profile>
struct FieldFactory<SWMRSnapshotMetadata<Profile> > {
private:
    using Type = SWMRSnapshotMetadata<Profile>;
    using SnapshotID = ApiProfileSnapshotID<Profile>;

public:
    static void serialize(SerializationData& data, const Type& field)
    {
        FieldFactory<SnapshotID>::serialize(data, field.parent_snapshot_id());
        FieldFactory<uint64_t>::serialize(data, field.superblock_file_pos());
        FieldFactory<uint64_t>::serialize(data, field.flags());
        FieldFactory<uint64_t>::serialize(data, field.timestamp());
        FieldFactory<uint64_t>::serialize(data, field.ttl());
    }

    static void serialize(SerializationData& data, const Type* field, int32_t count = 1){
        for (int32_t c = 0; c < count; c++){
            serialize(data, field[c]);
        }
    }

    static void deserialize(DeserializationData& data, Type& field)
    {
        SnapshotID snapshot_id{};
        FieldFactory<SnapshotID>::deserialize(data, snapshot_id);

        uint64_t file_pos{};
        FieldFactory<uint64_t>::deserialize(data, file_pos);

        uint64_t flags{};
        FieldFactory<uint64_t>::deserialize(data, flags);

        uint64_t timestamp{};
        FieldFactory<uint64_t>::deserialize(data, timestamp);

        uint64_t ttl{};
        FieldFactory<uint64_t>::deserialize(data, ttl);

        field.set_parent_snapshot_id(snapshot_id);
        field.set_superblock_file_pos(file_pos);
        field.set_flags(flags);
        field.set_timestamp(timestamp);
        field.ttl_ = ttl;
    }


    static void deserialize(DeserializationData& data, Type* field, int32_t count = 1){
        for (int32_t c = 0; c < count; c++){
            deserialize(data, field[c]);
        }
    }
};


template <typename Profile>
struct TypeHash<SWMRSnapshotMetadata<Profile>>: UInt64Value<
    HashHelper<
        459350639468,
        TypeHashV<ApiProfileSnapshotID<Profile>>,
        TypeHashV<uint64_t>,
        TypeHashV<uint64_t>,
        SWMRSnapshotMetadata<Profile>::VERSION,
        TypeHashV<Profile>
    >
> {};


template <typename ProfileDT>
struct SnapshotMetadataDT {};

template <typename ProfileDT>
struct TypeHash<SnapshotMetadataDT<ProfileDT>>: UInt64Value<
    HashHelper<
        32495987114593,
        TypeHashV<SWMRSnapshotMetadata<ProfileFromDataType<ProfileDT>>>
    >
> {};

template <typename ProfileDT>
struct DataTypeTraits<SnapshotMetadataDT<ProfileDT>>: FixedSizeDataTypeTraits<SWMRSnapshotMetadata<ProfileFromDataType<ProfileDT>>, SnapshotMetadataDT<ProfileDT>> {

    using Parameters = TL<ProfileDT>;

    static void create_signature(SBuf& buf, const SnapshotMetadataDT<ProfileDT>&) {
        create_signature(buf);
    }

    static void create_signature(SBuf& buf) {
        buf << "SnapshotMetadataDT<";
        DataTypeTraits<ProfileDT>::create_signature(buf);
        buf << ">";
    }
};

}
