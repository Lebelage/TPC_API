#include <memory>
#include <print>
#include <vector>
#include <expected>
#include <numeric>

#include <exec/start_detached.hpp>
#include <stdexec/execution.hpp>

#include <open62541pp/client.hpp>
#include "tpc_system/client/client.hpp"
#include "tpc_system/client/helpers/opcua_browse_adapter.hpp"
#include "tpc_core/definitions/client_definitions.hpp"
#include "tpc_system/models/data.hpp"
namespace tpc::system::client {

#pragma region Fabric/Constructor

[[nodiscard]] std::expected<std::unique_ptr<Client>, std::string> Client::create(std::string endpoint) {
    try {
        return std::unique_ptr<Client>{new Client(endpoint)};
    } catch (const std::exception& error) {
        return std::unexpected{
            std::format("[{}]: Failed to create TPC client: {}", core::definitions::CLIENT_ERROR__, error.what())};
    } catch (...) {
        return std::unexpected(
            std::format("[{}]: Failed to create TPC client: unknown error", core::definitions::CLIENT_ERROR__));
    };
};

Client::Client(std::string endpoint)
    : client_{std::make_unique<opcua::Client>()}, processing_pool_(2), endpoint_(endpoint) {
    auto subscription_result = Subscription::create();

    if (!subscription_result)
        return;

    subscription_ = std::move(subscription_result.value());

    subscription_->data_received_.subscribe([this](opcua::NodeId channel, opcua::DataValue value) {
        on_subscription_data_received(channel, value);
    });

    subscription_->error_occurred_.subscribe([this](std::string str) {
        on_subscription_error_occurred(str);
    });

    subscription_->info_occurred_.subscribe([this](std::string str) {
        on_subscription_info_occurred(str);
    });

    auto frame_receiver_result = FrameReceiver::create(36);

    if (!frame_receiver_result)
        return;

    frame_receiver_ = std::move(frame_receiver_result.value());
}

#pragma endregion

#pragma region Public methods

std::expected<bool, std::string> Client::connect_async() {
    bool expected = false;

    if (!running_.compare_exchange_strong(expected, true))
        return false;

    stop_requested_ = false;

    auto result = initialize_opcua_handlers();

    if (!result)
        return false;

    opcua_thread_ = std::jthread([this](std::stop_token stop_token) noexcept {
        try {
            if (!client_)
                throw std::runtime_error{"OPC UA client is not initialized"};

            if (endpoint_.empty())
                throw std::runtime_error{"OPC UA endpoint is empty"};

            client_->connectAsync(endpoint_);

            while (!stop_token.stop_requested() && !stop_requested_.load()) {
                client_->runIterate(polling_interval_ms_);
            }

            if (client_->isConnected())
                client_->disconnect();
        } catch (const std::exception& ex) {
            error_occurred_.invoke(ex.what());
        } catch (...) {
            error_occurred_.invoke("Unknown error in the OPC UA worker");
        }

        running_ = false;
    });
    return true;
}

bool Client::is_running() const {
    return running_.load();
}

void Client::stop() {
    stop_requested_ = true;
    opcua_thread_.request_stop();
}

auto Client::get_frame() -> std::optional<std::unordered_map<std::string, double>> {
    auto frame_result = frame_receiver_->get_frame();

    if (!frame_result)
        return std::nullopt;

    std::unordered_map<std::string, double> frame{};

    for (auto key : frame_result.value() | std::views::keys) {
        frame.insert_or_assign(channels_info.nodes.find(key)->second, frame_result.value().at(key));
    }

    return frame;
}

#pragma endregion

#pragma region Private methods

auto Client::initialize_opcua_handlers() -> std::expected<void, std::string> {
    if (!client_)
        return std::unexpected("Client is not initialized");

    client_->onSessionActivated([this] {
        initialize_monitored_items();
        on_opcua_session_activated();
    });
    client_->onConnected([this] {
        on_opcua_connected();
    });
    client_->onInactive([this] {
        on_opcua_inactive();
    });
    client_->onDisconnected([this] {
        on_opcua_disconnected();
    });
    client_->onSessionClosed([this] {
        on_opcua_session_closed();
    });

    return {};
}

#pragma region Helpers

auto Client::find_child(const opcua::BrowseResult& result, std::string_view name, opcua::NodeClass expected_class)
    -> std::optional<opcua::NodeId> {
    for (const auto& reference : result.references()) {
        if (!reference.isForward() || reference.nodeClass() != expected_class || !reference.nodeId().isLocal())
            continue;

        if (reference.browseName().name() == name)
            return reference.nodeId().nodeId();
    }

    return std::nullopt;
}

auto Client::find_slot_ids(const opcua::BrowseResult& result) -> std::vector<opcua::NodeId> {
    std::vector<opcua::NodeId> ids;

    for (const auto& reference : result.references()) {
        if (!reference.isForward() || reference.nodeClass() != opcua::NodeClass::Object
            || !reference.nodeId().isLocal())
            continue;

        if (reference.browseName().name().starts_with("Slot "))
            ids.push_back(reference.nodeId().nodeId());
    };
    return ids;
}

auto Client::append_channels(tpc::system::models::DiscoveryResult& result, const opcua::BrowseResult& slot_result)
    -> void {
    for (const auto& reference : slot_result.references()) {
        if (!reference.isForward() || reference.nodeClass() != opcua::NodeClass::Variable
            || !reference.nodeId().isLocal()) {
            continue;
        }

        result.nodes.emplace(reference.nodeId().nodeId(), std::string{reference.browseName().name()});
    }
}

#pragma endregion

#pragma region Handlers

auto Client::on_subscription_data_received(opcua::NodeId node, opcua::DataValue value) -> void {
    const auto values = value.value().array<double>();

    double average = std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(values.size());

    frame_receiver_->add_back(node, average);
}

auto Client::on_subscription_error_occurred(std::string message) -> void {
    error_occurred_.invoke(std::format("[{}]: {}", core::definitions::CLIENT_ERROR__, message));
}

auto Client::on_subscription_info_occurred(std::string message) -> void {
    info_occurred_.invoke(std::format("[{}]: {}", core::definitions::CLIENT_INFO__, message));
}

auto Client::on_opcua_session_activated() -> void {
    connection_state_changed_.invoke(ConnectionState::Connected);
}

auto Client::on_opcua_connected() -> void {
    connection_state_changed_.invoke(ConnectionState::Connected);
}

auto Client::on_opcua_inactive() -> void {
    connection_state_changed_.invoke(ConnectionState::Inactive);
}

auto Client::on_opcua_session_closed() -> void {
    connection_state_changed_.invoke(ConnectionState::SessionClosed);
}

auto Client::on_opcua_disconnected() -> void {
    connection_state_changed_.invoke(ConnectionState::Disconnected);
}

#pragma endregion

#pragma endregion

} // namespace tpc::system::client
