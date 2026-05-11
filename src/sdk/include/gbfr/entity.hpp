// SPDX-License-Identifier: MIT
// gbfr/entity.hpp — `Entity` wrapper.
//
// `Entity` has no virtual methods (no vftable was emitted), so the wrapper
// here is just a typed view over a heap object addressed by `EntityHandle`.
#pragma once

#include "common.hpp"
#include "object.hpp"

namespace gbfr {

class Entity : public GameObject {
public:
    using GameObject::GameObject;

    static constexpr std::ptrdiff_t kObjIdOffset =
        GBFR_TODO_OFFSET("Entity: offset of eObjId tag");
    static constexpr std::ptrdiff_t kPositionOffset =
        GBFR_TODO_OFFSET("Entity: offset of cVec4 world position");
    static constexpr std::ptrdiff_t kHandleOffset =
        GBFR_TODO_OFFSET("Entity: offset of self EntityHandle");

    [[nodiscard]] ObjId obj_id() const { return read<ObjId>(kObjIdOffset); }
    [[nodiscard]] EntityHandle handle() const { return read<EntityHandle>(kHandleOffset); }
};

} // namespace gbfr
