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
#include "../signatures.hpp"

namespace gbfr::managers {

class SaveDataInitModule : public GameObject {
public:
    using GameObject::GameObject;
    static constexpr Address kVftableRva = signatures::vft::kSaveDataInitModule;
};

class SaveDataReadModule : public GameObject {
public:
    using GameObject::GameObject;
    static constexpr Address kVftableRva = signatures::vft::kSaveDataReadModule;

    static constexpr Address kTaskRequestDeserializeSlotDataRva = 0; // TODO: address of task lambda
};

class SaveDataWriteModule : public GameObject {
public:
    using GameObject::GameObject;
    static constexpr Address kVftableRva = signatures::vft::kSaveDataWriteModule;

    // Each task is a `std::_Func_impl_no_alloc<lambda, void>`. Vftable RVAs:
    static constexpr Address kTaskRequestBuildCommonDataRva          = signatures::vft::kTaskRequestBuildCommonData;
    static constexpr Address kTaskRequestBuildGraphicsSettingDataRva = signatures::vft::kTaskRequestBuildGraphicsSettingData;
    static constexpr Address kTaskEntryWriteSlotDataRva              = signatures::vft::kTaskEntryWriteSlotData;
    static constexpr Address kTaskEntryWriteSlotInfoRva              = signatures::vft::kTaskEntryWriteSlotInfo;
};

class SaveDataDeleteModule : public GameObject {
public:
    using GameObject::GameObject;
    static constexpr Address kVftableRva = signatures::vft::kSaveDataDeleteModule;
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
