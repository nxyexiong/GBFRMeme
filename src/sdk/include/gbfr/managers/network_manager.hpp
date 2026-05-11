// SPDX-License-Identifier: MIT
// gbfr/managers/network_manager.hpp
//
// `NetworkSystemRpcManager` has a vftable (RVA 0x1447fe5e0).
// `Network` is the namespace that holds the two init state machines:
//   - Network::InitNetworkState   (vftable @ 0x1447fe778)
//   - Network::InitMultiPlayState (vftable @ 0x1447fe9c8)
#pragma once

#include "../object.hpp"

namespace gbfr::managers {

class NetworkSystemRpcManager : public GameObject {
public:
    using GameObject::GameObject;
    static constexpr std::string_view kSingletonName = "NetworkSystemRpcManager";
    static constexpr Address kVftableRva = 0x1447fe5e0ULL;
};

class InitNetworkState : public GameObject {
public:
    using GameObject::GameObject;
    static constexpr Address kVftableRva = 0x1447fe778ULL;
    // hw::network::StateMachine<Network::InitNetworkState> with N=2 states.
};

class InitMultiPlayState : public GameObject {
public:
    using GameObject::GameObject;
    static constexpr Address kVftableRva = 0x1447fe9c8ULL;
    // hw::network::StateMachine<Network::InitMultiPlayState> with N=2 states.
};

} // namespace gbfr::managers
