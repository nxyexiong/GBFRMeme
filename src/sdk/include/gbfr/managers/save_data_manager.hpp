// SPDX-License-Identifier: MIT
// gbfr/managers/save_data_manager.hpp
//
// SaveDataManager has no vftable; it dispatches to four polymorphic modules,
// each of which DOES have a vftable (RVAs below).
//
// Slot-write tasks discovered in symbol names:
//   SaveDataWriteModule::taskRequestBuildCommonData_
//   SaveDataWriteModule::taskRequestBuildGraphicsSettingData_
//   SaveDataWriteModule::taskEntryWriteSlotData_
//   SaveDataWriteModule::taskEntryWriteSlotInfo_
// Slot-read task:
//   SaveDataReadModule::taskRequestDeserializeSlotData_
#pragma once

#include "../object.hpp"

namespace gbfr::managers {

class SaveDataInitModule : public GameObject {
public:
    using GameObject::GameObject;
    static constexpr Address kVftableRva = 0x145153780ULL; // see vft_SaveDataInitModule.txt
};

class SaveDataReadModule : public GameObject {
public:
    using GameObject::GameObject;
    static constexpr Address kVftableRva = 0x1451537c0ULL; // see vft_SaveDataReadModule.txt

    static constexpr Address kTaskRequestDeserializeSlotDataRva = 0; // TODO: address of task lambda
};

class SaveDataWriteModule : public GameObject {
public:
    using GameObject::GameObject;
    static constexpr Address kVftableRva = 0x1451537a0ULL; // see vft_SaveDataWriteModule.txt

    // Each task is a `std::_Func_impl_no_alloc<lambda, void>`. Vftable RVAs:
    static constexpr Address kTaskRequestBuildCommonDataRva         = 0x144820ff8ULL;
    static constexpr Address kTaskRequestBuildGraphicsSettingDataRva = 0x1448210b8ULL;
    static constexpr Address kTaskEntryWriteSlotDataRva             = 0x144821158ULL;
    static constexpr Address kTaskEntryWriteSlotInfoRva             = 0x1448211f8ULL;
};

class SaveDataDeleteModule : public GameObject {
public:
    using GameObject::GameObject;
    static constexpr Address kVftableRva = 0x1451537e0ULL; // see vft_SaveDataDeleteModule.txt
};

// The orchestrator: no vftable, dispatches via a `unordered_map<string_hash32,
// SaveDataManager::CallData>`.
class SaveDataManager : public GameObject {
public:
    using GameObject::GameObject;

    static constexpr std::string_view kSingletonName = "SaveDataManager";

    [[nodiscard]] SaveDataInitModule    init_module()    const { return read_object_ptr<SaveDataInitModule>(GBFR_TODO_OFFSET("SaveDataManager::init_module ptr")); }
    [[nodiscard]] SaveDataReadModule    read_module()    const { return read_object_ptr<SaveDataReadModule>(GBFR_TODO_OFFSET("SaveDataManager::read_module ptr")); }
    [[nodiscard]] SaveDataWriteModule   write_module()   const { return read_object_ptr<SaveDataWriteModule>(GBFR_TODO_OFFSET("SaveDataManager::write_module ptr")); }
    [[nodiscard]] SaveDataDeleteModule  delete_module()  const { return read_object_ptr<SaveDataDeleteModule>(GBFR_TODO_OFFSET("SaveDataManager::delete_module ptr")); }
};

} // namespace gbfr::managers
