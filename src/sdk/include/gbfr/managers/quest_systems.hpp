// SPDX-License-Identifier: MIT
// gbfr/managers/quest_systems.hpp
//
// stage::quest namespace — Steam build 24245499 vftables.
//   ProgressManager         @ 0x1454b8da8
//   MainQuestManager        @ 0x146139b70
//   MultiQuestManager       @ 0x146139c70
//   FateEpisodeManager      @ 0x146139cf0
//   BaseTownQuestManager    @ 0x146139d70
//   TrialBattleManager      @ 0x146139df0
//   ChallengeMissionManager @ 0x146139e70
//   ShortStoryQuestManager  @ 0x146139ef0
//
// QuestSystem is the umbrella runtime; both it and ProgressManager appear
// only through `cyan::Singleton<T>` (and ProgressManager has a vftable, the
// rest of the *Manager classes derive from `QuestManagerBase`).
#pragma once

#include "../object.hpp"
#include "../signatures.hpp"

namespace gbfr::managers::quest {

class QuestSystem : public GameObject {
public:
    using GameObject::GameObject;
    static constexpr std::string_view kSingletonName = "stage::quest::QuestSystem";
};

class ProgressManager : public GameObject {
public:
    using GameObject::GameObject;
    static constexpr std::string_view kSingletonName = "stage::quest::ProgressManager";
    static constexpr Address kVftableRva = signatures::vft::kProgressManager;
};

class QuestManagerBase : public GameObject {
public:
    using GameObject::GameObject;
};

class MainQuestManager : public QuestManagerBase {
public:
    using QuestManagerBase::QuestManagerBase;
    static constexpr Address kVftableRva = signatures::vft::kMainQuestManager;
};

class MultiQuestManager : public QuestManagerBase {
public:
    using QuestManagerBase::QuestManagerBase;
    static constexpr Address kVftableRva = signatures::vft::kMultiQuestManager;
};

class FateEpisodeManager : public QuestManagerBase {
public:
    using QuestManagerBase::QuestManagerBase;
    static constexpr Address kVftableRva = signatures::vft::kFateEpisodeManager;
};

class BaseTownQuestManager : public QuestManagerBase {
public:
    using QuestManagerBase::QuestManagerBase;
    static constexpr Address kVftableRva = signatures::vft::kBaseTownQuestManager;
};

class TrialBattleManager : public QuestManagerBase {
public:
    using QuestManagerBase::QuestManagerBase;
    static constexpr Address kVftableRva = signatures::vft::kTrialBattleManager;
};

class ChallengeMissionManager : public QuestManagerBase {
public:
    using QuestManagerBase::QuestManagerBase;
    static constexpr Address kVftableRva = signatures::vft::kChallengeMissionManager;
};

class ShortStoryQuestManager : public QuestManagerBase {
public:
    using QuestManagerBase::QuestManagerBase;
    static constexpr Address kVftableRva = signatures::vft::kShortStoryQuestManager;
};

} // namespace gbfr::managers::quest
