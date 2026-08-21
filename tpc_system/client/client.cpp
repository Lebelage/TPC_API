module;
#include <exec/start_detached.hpp>
#include <open62541pp/client.hpp>
#include <stdexec/execution.hpp>
module tpc.system.client;
import tpc.system.client.helpers.async_adapters.opcua_browse_adapter;

namespace tpc::system::client {

#pragma region Fabric/Constructor

[[nodiscard]] std::expected<std::unique_ptr<Client>, std::string> Client::create(std::string endpoint) {
    try {
        return std::unique_ptr<Client>{new Client(endpoint)};
    } catch (const std::exception& error) {
        return std::unexpected{std::format("Failed to create TPC client: {}", error.what())};
    } catch (...) {
        return std::unexpected{"Failed to create TPC client: unknown error"};
    }
};

Client::Client(std::string endpoint)
    : client_{std::make_unique<opcua::Client>()}, opcua_pool_(1), processing_pool_(2), endpoint_(endpoint) {
    auto result = Subscription::create();

    if (!result)
        return;

    subscription_ = std::move(result.value());
    subscription_->error_occurred_.subscribe([](std::string str) {std::cout << str << std::endl; });
}

#pragma endregion

#pragma region Public methods

std::expected<bool, std::string> Client::connect_async() {
    bool expected = false;

    if (!running_.compare_exchange_strong(expected, true))
        return false;

    stop_requested_ = false;

    initialize_handlers();

    auto task = stdexec::schedule(opcua_pool_.get_scheduler()) | stdexec::then([this] {
                    if (!client_)
                        throw std::runtime_error{"OPC UA client is not initialized"};

                    if (endpoint_.empty())
                        throw std::runtime_error{"OPC UA endpoint is empty"};

                    client_->connectAsync(endpoint_);

                    while (!stop_requested_.load()) {
                        client_->runIterate(polling_interval_ms_);
                    }

                    if (client_->isConnected())
                        client_->disconnectAsync();
                })

                | stdexec::then([this] {
                      running_ = false;
                  })
                | stdexec::upon_error([this](std::exception_ptr error) noexcept {
                      running_ = false;

                      try {
                          std::rethrow_exception(error);
                      } catch (const std::exception& ex) {
                          std::cerr << "TPC client error: " << ex.what() << '\n';
                      }
                  });

    exec::start_detached(std::move(task));
    return true;
}

bool Client::is_running() const {
    return running_.load();
}

void Client::stop() {
    stop_requested_ = true;
}

#pragma endregion

#pragma region Private methods

auto Client::initialize_monitored_items() -> std::expected<void, std::string> {
    auto task = stdexec::starts_on(stdexec::inline_scheduler{}, discover_folders())
                | stdexec::let_value([this](models::DiscoveryResult discovery) {
                    channels_info = std::move(discovery);

                    return create_subscription(channels_info);
                })
                | stdexec::upon_error([this](std::exception_ptr error) noexcept {
                    try {
                        std::rethrow_exception(error);
                    } catch (const std::exception& ex) {
                        //report_error(ex.what());
                    }
                });

    exec::start_detached(std::move(task));

    return {};
}

auto Client::initialize_opcua_handlers() -> std::expected<void, std::string> {
    if (!client_)
        return std::unexpected("Client is not initialized");

    client_->onSessionActivated([this] {
        initialize_monitored_items();
    });

    client_->onSessionClosed([] {
        std::cout << "OPC UA session closed\n";
    });

    client_->onInactive([this] {
        // report_error("OPC UA client became inactive");
    });

    return {};
}

auto Client::create_subscription(models::DiscoveryResult& discovery) -> stdexec::task<void> {
    if (!subscription_)
        throw std::runtime_error("Subscription is null");

    subscription_->create_subscription(*client_, discovery.channels);

    co_return;
}

auto Client::start_discovery() -> std::expected<void, std::string> {
    auto task = stdexec::starts_on(stdexec::inline_scheduler{}, discover_folders());

    exec::start_detached(std::move(task));
}

auto Client::discover_folders() -> stdexec::task<models::DiscoveryResult> {
    models::DiscoveryResult result;

    auto root =
        co_await tpc::system::client::helpers::browse_async(*client_, opcua::NodeId{opcua::ObjectId::ObjectsFolder});

    auto target_channel_id = find_child(root, "ADC Channels", opcua::NodeClass::Object);

    if (!target_channel_id) {
        throw std::runtime_error{"Cannot find target channel ID"};
    }

    auto slots_result = co_await tpc::system::client::helpers::browse_async(*client_, std::move(*target_channel_id));

    auto slot_ids = find_slot_ids(slots_result);

    if (slot_ids.empty())
        throw std::runtime_error{"No slots found"};

    for (auto& slot_id : slot_ids) {
        auto slot_result = co_await tpc::system::client::helpers::browse_async(*client_, std::move(slot_id));

        append_channels(result, slot_result);
    }
    co_return result;
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
        result.channels.push_back(reference.nodeId().nodeId());
        result.names.push_back(std::string{reference.browseName().name()});

        ///
        std::cout << reference.browseName().name() << "\n";
    }
}

#pragma endregion

#pragma endregion

} // namespace tpc::system::client
