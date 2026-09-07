module;

#include <cstddef>
#include <open62541pp/client.hpp>
#include <string>
#include <unordered_map>

export module tpc.system.models.system_data;

export namespace tpc::system::models {

struct NodeIdHash {
    std::size_t operator()(const opcua::NodeId& node_id) const noexcept {
        return static_cast<std::size_t>(node_id.hash());
    }
};

struct NodeIdEqual {
    bool operator()(const opcua::NodeId& lhs, const opcua::NodeId& rhs) const noexcept {
        return lhs == rhs;
    }
};

struct DiscoveryResult {
    std::unordered_map<opcua::NodeId, std::string, NodeIdHash, NodeIdEqual> nodes;
};

struct ReceivedItem {
    std::string name{};
    double value{};
};

}  // namespace tpc::system::models