module;
#include "../../../../../dev/vcpkg/installed/arm64-osx/include/open62541pp/services/detail/client_service.hpp"
#include "../../../../../dev/vcpkg/installed/arm64-osx/include/open62541pp/services/subscription.hpp"
#include "open62541pp/services/monitoreditem.hpp"
export module tpc.system.client.helpers.subscription;
import std;
import tpc.system.client.event_handler;

export namespace tpc::system::client::helpers {

struct DefaultSubscriptionConfig {
    constexpr static double publishing_interval = 1000.;

    constexpr static uint32_t max_keep_alive_count = 10;
};

class Subscription {
public:
    [[nodiscard]] std::expected<std::unique_ptr<Subscription>, std::string> create() {
        try {
            opcua::services::SubscriptionParameters parameters{
                .publishingInterval = DefaultSubscriptionConfig::publishing_interval,
                .maxKeepAliveCount = DefaultSubscriptionConfig::max_keep_alive_count};

            return std::make_unique<Subscription>{new Subscription(parameters)};
        } catch (...) {
            return std::unexpected("Failed to create subscription: unknown error");
        }
    }

    ~Subscription() = default;

    Subscription(const Subscription&) = delete;
    Subscription& operator=(const Subscription&) = delete;
    Subscription(Subscription&&) = delete;
    Subscription& operator=(Subscription&&) = delete;

private:
    Subscription(opcua::services::SubscriptionParameters parameters) : parameters_(parameters) {}

public:
private:
    /// Need to TODO
    std::expected<void, std::string> create_subscription(opcua::Client& client,
                                                         std::span<const opcua::NodeId> channels_id) {
        opcua::services::createSubscriptionAsync(
            client, parameters_, true, nullptr,
            [this](opcua::IntegerId subscription_id) {
                Release(subscription_id);
            },
            [this, &client, channels_id](opcua::CreateSubscriptionResponse& response) noexcept {
                create_monitored_items(client, response, channels_id);
            });

        return {};
    }

    std::expected<void, std::string> create_monitored_items(opcua::Client& client,
                                                            opcua::CreateSubscriptionResponse& response,
                                                            std::span<const opcua::NodeId> channels_id) {
        try {
            opcua::services::MonitoringParametersEx parameters{
                .samplingInterval = 1000, .queueSize = 1, .discardOldest = true};

            response.responseHeader().serviceResult().throwIfBad();

            subscription_id_ = response.subscriptionId();

            for (const auto& channel : channels_id) {
                auto read_value = opcua::ReadValueId{channel, opcua::AttributeId::Value};
                opcua::services::createMonitoredItemDataChangeAsync(
                    client, *subscription_id_, read_value, opcua::MonitoringMode::Reporting, parameters,
                    [this, channel](opcua::IntegerId, opcua::IntegerId, const opcua::DataValue& value) noexcept {
                        try {
                            data_received_(channel, value);
                        } catch (std::exception& ex) {
                            error_occurred_(ex.what());
                        }
                    },
                    [this, channel](opcua::IntegerId id, opcua::IntegerId monId) noexcept {
                        error_occurred_.emit(std::format("Monitored item was deleted: {}", channel.toString()));
                    },
                    [this, channel](opcua::MonitoredItemCreateResult& result) noexcept {
                        try {
                            result.statusCode().throwIfBad();

                            monitored_item_ids_.push_back(result.monitoredItemId());
                        } catch (const std::exception& error) {
                            error_occurred_.emit(
                                std::format("Cannot monitor {}: {}", channel.toString(), error.what()));
                        }
                    });
            }

        } catch (std::exception& ex) {
            error_occurred_.emit(ex.what());
        }
    }

private:
private:
    opcua::services::SubscriptionParameters parameters_;

    std::optional<opcua::IntegerId> subscription_id_;
    std::vector<opcua::IntegerId> monitored_item_ids_;

private:
    void Release(opcua::IntegerId subscription_id) noexcept {
        if (subscription_id_ == subscription_id) {
            subscription_id_.reset();
            monitored_item_ids_.clear();
        }
    }

public:
    EventHandler<std::string> error_occurred_;
    EventHandler<opcua::NodeId, opcua::DataValue> data_received_;
};
} // namespace tpc::system::client::helpers