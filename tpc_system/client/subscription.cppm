module;
#include "open62541pp/services/detail/client_service.hpp"
#include "open62541pp/services/monitoreditem.hpp"
#include "open62541pp/services/subscription.hpp"
export module tpc.system.client.subscription;
import std;
import tpc.system.client.event_handler;

export namespace tpc::system::client {

struct DefaultSubscriptionConfig;

class Subscription {
public:
    [[nodiscard]] static std::expected<std::unique_ptr<Subscription>, std::string> create();

    ~Subscription() = default;

    Subscription(const Subscription&) = delete;
    Subscription& operator=(const Subscription&) = delete;
    Subscription(Subscription&&) = delete;
    Subscription& operator=(Subscription&&) = delete;

private:
    Subscription(opcua::services::SubscriptionParameters parameters);

public:
    /// Need to TODO
    std::expected<void, std::string> create_subscription(opcua::Client& client,
                                                         std::span<const opcua::NodeId> channels_id);



    std::expected<void, std::string> create_monitored_items(opcua::Client& client,
                                                            opcua::CreateSubscriptionResponse& response,
                                                            std::span<const opcua::NodeId> channels_id);

private:
    opcua::services::SubscriptionParameters parameters_;

    std::optional<opcua::IntegerId> subscription_id_;
    std::vector<opcua::IntegerId> monitored_item_ids_;

private:
    void Release(opcua::IntegerId subscription_id) noexcept;
public:
    EventHandler<std::string> error_occurred_;
    EventHandler<opcua::NodeId, opcua::DataValue> data_received_;
};
} // namespace tpc::system::client