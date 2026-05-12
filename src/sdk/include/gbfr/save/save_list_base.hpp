// SPDX-License-Identifier: MIT
// gbfr/save/save_list_base.hpp — fixed-capacity hash list templates.
//
// Mirrors:
//   table::SaveListBase<TData, KeyType, MaxCount>
//   table::SaveTempListBase<TData, KeyType, MaxCount, ChunkCount>
//
// Layout (verified for CharaList):
//   +0x00       vftable pointer
//   +0x10       entries array of MaxCount * EntryStride bytes
//
// Each entry has a per-row header before the user-visible fields. Per
// `signatures::offset::chara_data`:
//   entry + 0x00  unknown
//   entry + 0x08  liveness sentinel (qword == 0xFFFFFFFFFFFFFFFF when empty)
//   entry + 0x10  Key (e.g. string_hash32 of character id)
//   entry + 0x18  ... TData fields
//
// `EntryStride` defaults to `sizeof(TData)` but lists with a verified size
// (e.g. CharaList = 0x3120) override it.
#pragma once

#include "../common.hpp"
#include "../object.hpp"
#include "../signatures.hpp"

namespace gbfr::save {

template <class TData, class Key, std::size_t MaxCount,
          std::size_t EntryStride = sizeof(TData)>
class SaveListBase : public GameObject {
public:
    using GameObject::GameObject;

    using value_type = TData;
    using key_type   = Key;

    static constexpr std::size_t kMaxCount    = MaxCount;
    static constexpr std::size_t kEntryStride = EntryStride;

    static constexpr std::ptrdiff_t kEntriesArrayOffset =
        signatures::offset::save_list_base::kEntriesArray;

    static constexpr std::uint64_t kEmptyQword = 0xFFFFFFFFFFFFFFFFull;

    // Absolute address of slot `i`'s entry.
    [[nodiscard]] Address entry_address(std::size_t i) const {
        return address() + static_cast<Address>(
            kEntriesArrayOffset + i * kEntryStride);
    }

    // True if slot `i` holds a real entry. The engine fills empty slots'
    // liveness qword (entry+0x08) with `0xFFFFFFFFFFFFFFFF`.
    [[nodiscard]] bool is_live(std::size_t i) const {
        return Memory::read<std::uint64_t>(memory(),
            entry_address(i) +
            static_cast<Address>(signatures::offset::chara_data::kLivenessQword))
            != kEmptyQword;
    }

    // Read slot `i`'s key.
    [[nodiscard]] Key key_at(std::size_t i) const {
        return Memory::read<Key>(memory(),
            entry_address(i) +
            static_cast<Address>(signatures::offset::chara_data::kKey));
    }

    // Read a typed field at `offset` inside slot `i`'s entry.
    template <class T>
    [[nodiscard]] T field_at(std::size_t i, std::ptrdiff_t offset) const {
        return Memory::read<T>(memory(),
            entry_address(i) + static_cast<Address>(offset));
    }

    // Count live entries.
    [[nodiscard]] std::uint32_t live_count() const {
        std::uint32_t n = 0;
        for (std::size_t i = 0; i < kMaxCount; ++i) {
            if (is_live(i)) ++n;
        }
        return n;
    }
};

// `table::SaveTempListBase<TData, Key, MaxCount, ChunkCount>` — used by
// `sys::data::GemList` (Cap 5100, Chunk 999) and
// `sys::data::ItemPendulumList` (Cap 5000, Chunk 5000). Internal storage is
// chunked; not yet reverse-engineered.
template <class TData, class Key, std::size_t MaxCount, std::size_t ChunkCount>
class SaveTempListBase : public GameObject {
public:
    using GameObject::GameObject;

    using value_type = TData;
    using key_type   = Key;

    static constexpr std::size_t kMaxCount   = MaxCount;
    static constexpr std::size_t kChunkCount = ChunkCount;
};

} // namespace gbfr::save
