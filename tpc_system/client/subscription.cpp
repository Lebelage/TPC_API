module tpc.system.client.subscription;
#include "open62541pp/services/subscription.hpp"

#include "open62541pp/services/monitoreditem.hpp"

namespace tpc::system::client {

struct DefaultSubscriptionConfig {
    constexpr static double publishing_interval = 1000.;
    constexpr static uint32_t max_keep_alive_count = 10;
};

#pragma region Constructor/Fabric

std::expected<std::unique_ptr<Subscription>, std::string> Subscription::create() {
    try {
        opcua::services::SubscriptionParameters parameters{
            .publishingInterval = DefaultSubscriptionConfig::publishing_interval,
            .maxKeepAliveCount = DefaultSubscriptionConfig::max_keep_alive_count};

        return std::unique_ptr<Subscription>{new Subscription(std::move(parameters))};
    } catch (...) {
        return std::unexpected("Failed to create subscription: unknown error");
    }
}

Subscription::Subscription(opcua::services::SubscriptionParameters parameters) : parameters_(parameters) {}

#pragma endregion

#pragma region Public methods

std::expected<void, std::string> Subscription::create_subscription(opcua::Client& client,
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

std::expected<void, std::string> Subscription::create_monitored_items(opcua::Client& client,
                                                                      opcua::CreateSubscriptionResponse& response,
                                                                      std::span<const opcua::NodeId> channels_id) {
    try {
        opcua::services::MonitoringParametersEx parameters{
            .samplingInterval = 1000, .queueSize = 1, .discardOldest = true};

        response.responseHeader().serviceResult().throwIfBad();

        subscription_id_ = response.subscriptionId();

        for (const auto& channel : channels_id) {
            ///
            std::cout << "Создаю monitored item для: " << opcua::toString(channel) << '\n';

            auto read_value = opcua::ReadValueId{channel, opcua::AttributeId::Value};
            opcua::services::createMonitoredItemDataChangeAsync(
                client, *subscription_id_, read_value, opcua::MonitoringMode::Reporting, parameters,
                [this, channel](opcua::IntegerId, opcua::IntegerId, const opcua::DataValue& value) noexcept {
                    try {
                        data_received_(channel, value);
                        value.value().to<std::string>();
                        ///
                        std::cout << std::format("Monitored item was created: {}", value.value().to<std::string>())
                                  << "\n";

                    } catch (std::exception& ex) {
                        error_occurred_(ex.what());
                    }
                },
                [this, channel](opcua::IntegerId id, opcua::IntegerId monId) noexcept {
                    error_occurred_.emit(std::format("Monitored item was deleted: {}", channel.toString()));

                    ///
                    std::cout << std::format("Monitored item was deleted: {}", channel.toString()) << "\n";
                },
                [this, channel](opcua::MonitoredItemCreateResult& result) noexcept {
                    try {
                        result.statusCode().throwIfBad();

                        monitored_item_ids_.push_back(result.monitoredItemId());
                    } catch (const std::exception& error) {
                        error_occurred_.emit(std::format("Cannot monitor {}: {}", channel.toString(), error.what()));

                        ///
                        std::cout << std::format("Cannot monitor {}: {}", channel.toString(), error.what()) << "\n";
                    }
                });
        }

    } catch (std::exception& ex) {
        error_occurred_.emit(ex.what());
        std::cout << ex.what() << "\n";
    }

    return {};
}

#pragma endregion

#pragma region Private methods

void Subscription::Release(opcua::IntegerId subscription_id) noexcept {
    if (subscription_id_ == subscription_id) {
        subscription_id_.reset();
        monitored_item_ids_.clear();
    }
}

#pragma endregion
} // namespace tpc::system::client