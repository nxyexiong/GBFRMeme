// SPDX-License-Identifier: MIT
// gbfr/save/save_data_unit.hpp — leaf save fields (fixed-size POD arrays).
//
// In the binary these are MSVC RTTI types like
//     SaveDataUnit<unsigned int, 1>
//     SaveDataUnit<unsigned int, 5100>
//     SaveDataUnit<char16_t, 32>
//     SaveDataUnit<bool, 1536>
//     ...
//
// Confirmed instantiations are listed in `vftables_index.tsv`.
#pragma once

#include "../common.hpp"
#include "../object.hpp"

#include <array>

namespace gbfr::save {

// Generic mirror of `SaveDataUnit<T, N>`. Layout (assumed):
//   [0x00] vftable
//   [????] array of N x T  -- offset TODO
template <class T, std::size_t N>
class SaveDataUnit : public GameObject {
public:
    using GameObject::GameObject;

    static constexpr std::size_t kCount = N;

    static constexpr std::ptrdiff_t kArrayOffset =
        GBFR_TODO_OFFSET("SaveDataUnit<T,N>: offset of internal array");

    [[nodiscard]] T at(std::size_t index) const {
        return read<T>(kArrayOffset + static_cast<std::ptrdiff_t>(index * sizeof(T)));
    }
    bool set(std::size_t index, const T& value) const {
        return write<T>(kArrayOffset + static_cast<std::ptrdiff_t>(index * sizeof(T)), value);
    }
};

// `SaveDataUnitVariable<T>` — variable-count cousin. Cap and current size
// live at unknown offsets. The vftables of `SaveDataUnitVariable<int>`,
// `SaveDataUnitVariable<unsigned __int64>`, etc. are in `.rdata`.
template <class T>
class SaveDataUnitVariable : public GameObject {
public:
    using GameObject::GameObject;

    static constexpr std::ptrdiff_t kSizeOffset =
        GBFR_TODO_OFFSET("SaveDataUnitVariable<T>: live count offset");
    static constexpr std::ptrdiff_t kCapacityOffset =
        GBFR_TODO_OFFSET("SaveDataUnitVariable<T>: capacity offset");
    static constexpr std::ptrdiff_t kDataOffset =
        GBFR_TODO_OFFSET("SaveDataUnitVariable<T>: data pointer offset");

    [[nodiscard]] std::uint32_t size() const { return read<std::uint32_t>(kSizeOffset); }
    [[nodiscard]] std::uint32_t capacity() const { return read<std::uint32_t>(kCapacityOffset); }

    [[nodiscard]] T at(std::size_t index) const {
        Address data_ptr = read<Address>(kDataOffset);
        if (!data_ptr) return T{};
        return Memory::read<T>(memory(), data_ptr + index * sizeof(T));
    }
};

} // namespace gbfr::save
