// SPDX-License-Identifier: MIT
// gbfr/save/save_list_base.hpp — fixed-capacity hash list templates.
//
// Mirrors:
//   table::SaveListBase<TData, KeyType, MaxCount>
//   table::SaveTempListBase<TData, KeyType, MaxCount, ChunkCount>
//   table::HashList<TData, MaxCount>
//   table::UniqueList<TData, MaxCount, ChunkCount>
//
// All have a vftable at offset 0; the in-memory layout of the table itself
// (key array + value array + occupancy bitset + live count) is not yet
// recovered — see GBFR_TODO_OFFSET below.
#pragma once

#include "../common.hpp"
#include "../object.hpp"

namespace gbfr::save {

namespace detail {
template <class Key>
inline Key key_from_index(std::size_t i) {
    return static_cast<Key>(i);
}
template <>
inline StringHash32 key_from_index<StringHash32>(std::size_t i) {
    return StringHash32(static_cast<std::uint32_t>(i));
}
} // namespace detail

// `table::SaveListBase<TData, Key, MaxCount>` — fixed-capacity hash map of
// MaxCount slots keyed by `Key` (either `StringHash32` or `std::uint32_t`).
//
// TData is read by value, so it must be trivially copyable. The TData
// structs themselves still need their field offsets reverse-engineered.
template <class TData, class Key, std::size_t MaxCount>
class SaveListBase : public GameObject {
public:
    using GameObject::GameObject;

    using value_type = TData;
    using key_type   = Key;

    static constexpr std::size_t kMaxCount = MaxCount;

    static constexpr std::ptrdiff_t kKeyArrayOffset =
        GBFR_TODO_OFFSET("SaveListBase: offset of key array");
    static constexpr std::ptrdiff_t kValueArrayOffset =
        GBFR_TODO_OFFSET("SaveListBase: offset of value array");
    static constexpr std::ptrdiff_t kLiveCountOffset =
        GBFR_TODO_OFFSET("SaveListBase: offset of live entry count");

    [[nodiscard]] std::uint32_t live_count() const {
        return read<std::uint32_t>(kLiveCountOffset);
    }

    // Read the `i`-th slot's key (irrespective of whether the slot is in use).
    [[nodiscard]] Key key_at(std::size_t i) const {
        return read<Key>(kKeyArrayOffset + static_cast<std::ptrdiff_t>(i * sizeof(Key)));
    }

    // Read the `i`-th slot's value.
    [[nodiscard]] TData value_at(std::size_t i) const {
        return read<TData>(kValueArrayOffset + static_cast<std::ptrdiff_t>(i * sizeof(TData)));
    }

    // Address of the `i`-th value (for in-place mutation / pointer hand-off).
    [[nodiscard]] Address value_address(std::size_t i) const {
        return address() + static_cast<Address>(kValueArrayOffset + i * sizeof(TData));
    }
};

// `table::SaveTempListBase<TData, Key, MaxCount, ChunkCount>` — used by
// `sys::data::GemList` (Cap 5100, Chunk 999) and
// `sys::data::ItemPendulumList` (Cap 5000, Chunk 5000). Storage is split
// into multiple chunks (probably for chunked allocation / freelist
// management); chunks themselves are pointer-indirect.
template <class TData, class Key, std::size_t MaxCount, std::size_t ChunkCount>
class SaveTempListBase : public GameObject {
public:
    using GameObject::GameObject;

    using value_type = TData;
    using key_type   = Key;

    static constexpr std::size_t kMaxCount   = MaxCount;
    static constexpr std::size_t kChunkCount = ChunkCount;

    static constexpr std::ptrdiff_t kChunkTableOffset =
        GBFR_TODO_OFFSET("SaveTempListBase: chunk pointer table offset");
    static constexpr std::ptrdiff_t kLiveCountOffset =
        GBFR_TODO_OFFSET("SaveTempListBase: live count offset");

    [[nodiscard]] std::uint32_t live_count() const {
        return read<std::uint32_t>(kLiveCountOffset);
    }
};

} // namespace gbfr::save
