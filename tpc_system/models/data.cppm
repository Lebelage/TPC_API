module;
#include <open62541pp/client.hpp>
export module tpc.system.models.data;
export namespace tpc::system::models{
    struct Channel{
        std::string name;
        opcua::NodeId node_id;
    };

    struct DiscoveryState
        {
            std::size_t pending_slots{};
            std::vector<Channel> channels;
        };
}