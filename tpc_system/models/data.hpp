#pragma once
#include <open62541pp/client.hpp>
#include <vector>

namespace tpc::system::models {

/// Main opcua server information model
struct DiscoveryResult {

    ///Dictionary represents
    ///- [opcua::NodeId] Sensor`s NodeId
    ///- [std::string] Sensor`s name
    std::unordered_map<opcua::NodeId, std::string> nodes;
};

/// Received item data model
struct ReceivedItem {
    std::string name{};
    double value{};
};

} // namespace tpc::system::models