// SPDX-License-Identifier: MIT
// gbfr/managers/quest_systems.hpp
//
// stage::quest namespace — vftables in 0x145153cd8 .. 0x145153e98.
//   ProgressManager        @ 0x1447fc698
//   MainQuestManager       @ 0x145153cd8
//   MultiQuestManager      @ 0x145153d58
//   FateEpisodeManager     @ 0x145153d98
//   BaseTownQuestManager   @ 0x145153dd8
//   TrialBattleManager     @ 0x145153e18
//   ChallengeMissionManager @ 0x145153e58
//   ShortStoryQuestManager @ 0x145153e98
//
// QuestSystem is the umbrella runtime; both it and ProgressManager appear
// only through `cyan::Singleton<T>` (and ProgressManager has a vftable, the
// rest of the *Manager classes derive from `QuestManagerBase`).
#pragma once

#include "../object.hpp"

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
    static constexpr Address kVftableRva = 0x1447fc698ULL;
};

class QuestManagerBase : public GameObject {
public:
    using GameObject::GameObject;
};

class MainQuestManager : public QuestManagerBase {
public:
    using QuestManagerBase::QuestManagerBase;
    static constexpr Address kVftableRva = 0x145153cd8ULL;
};

class MultiQuestManager : public QuestManagerBase {
public:
    using QuestManagerBase::QuestManagerBase;
    static constexpr Address kVftableRva = 0x145153d58ULL;
};

class FateEpisodeManager : public QuestManagerBase {
public:
    using QuestManagerBase::QuestManagerBase;
    static constexpr Address kVftableRva = 0x145153d98ULL;
};

class BaseTownQuestManager : public QuestManagerBase {
public:
    using QuestManagerBase::QuestManagerBase;
    static constexpr Address kVftableRva = 0x145153dd8ULL;
};

class TrialBattleManager : public QuestManagerBase {
public:
    using QuestManagerBase::QuestManagerBase;
    static constexpr Address kVftableRva = 0x145153e18ULL;
};

class ChallengeMissionManager : public QuestManagerBase {
public:
    using QuestManagerBase::QuestManagerBase;
    static constexpr Address kVftableRva = 0x145153e58ULL;
};

class ShortStoryQuestManager : public QuestManagerBase {
public:
    using QuestManagerBase::QuestManagerBase;
    static constexpr Address kVftableRva = 0x145153e98ULL;
};

} // namespace gbfr::managers::quest
