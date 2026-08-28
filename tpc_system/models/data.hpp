#include <vector>
#include <open62541pp/client.hpp>
namespace tpc::system::models {

struct Channel {
    std::string   name;
    opcua::NodeId node_id;
};

struct DiscoveryState {
    std::size_t          pending_slots{};
    std::vector<Channel> channels;
};

struct DiscoveryResult {
    std::vector<opcua::NodeId> channels;
    std::vector<std::string> names;

    std::unordered_map<std::string, opcua::NodeId> nodes;
};
} // namespace tpc::system::models