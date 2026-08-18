module;
#include <exec/execute.hpp>
#include <exec/start_detached.hpp>
#include <exec/static_thread_pool.hpp>
#include <open62541pp/client.hpp>
#include <stdexec/__detail/__task.hpp>
#include <stdexec/execution.hpp>
export module tpc.system.tpc_client;

import std;
import tpc.system.client.helpers.async_adapters.opcua_browse_adapter;
import tpc.system.models.data;

export namespace tpc::system::tpc_client {

namespace models = tpc::system::models;

class TPCClient {
    // using UpdateHandler = std::function<void(ChannelUpdate)>;
    using ErrorHandler = std::function<void(std::string)>;

public:
    [[nodiscard]] static std::expected<std::unique_ptr<TPCClient>, std::string> create(std::string endpoint) {
        try {
            return std::unique_ptr<TPCClient>{new TPCClient(endpoint)};
        } catch (const std::exception& error) {
            return std::unexpected{std::format("Failed to create TPC client: {}", error.what())};
        } catch (...) {
            return std::unexpected{"Failed to create TPC client: unknown error"};
        }
    }

    ~TPCClient() = default;

    TPCClient(const TPCClient&)            = delete;
    TPCClient& operator=(const TPCClient&) = delete;
    TPCClient(TPCClient&&)                 = delete;
    TPCClient& operator=(TPCClient&&)      = delete;

private:
    TPCClient(std::string endpoint)
        : client_{std::make_unique<opcua::Client>()}, opcua_pool_(1), processing_pool_(2), endpoint_(endpoint) {}

public:
    std::expected<bool, std::string> connect_async() {
        bool expected = false;

        if (!running_.compare_exchange_strong(expected, true))
            return false;

        stop_requested_ = false;

        subscribe_handlers();

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

                    | stdexec::then([this] { running_ = false; })
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

    void stop() noexcept { stop_requested_ = true; }

    bool is_running() noexcept { return running_.load(); }

private:
    std::expected<void, std::string> subscribe_handlers() {
        if (!client_)
            return std::unexpected("Client is not initialized");

        client_->onSessionActivated([this] { start_discovery(); });

        client_->onSessionClosed([] { std::cout << "OPC UA session closed\n"; });

        client_->onInactive([this] { report_error("OPC UA client became inactive"); });

        return {};
    }

    void report_error(std::string message) noexcept {
        try {
            ErrorHandler handler;

            {
                std::scoped_lock lock{handlers_mutex_};
                handler = error_handler_;
            }

            if (handler)
                handler(std::move(message));
            else
                std::cerr << message << '\n';
        } catch (...) {
        }
    }

    // void set_update_handler(UpdateHandler handler)
    // {
    //     std::scoped_lock lock{handlers_mutex_};
    //     update_handler_ = std::move(handler);
    // }

    void set_error_handler(ErrorHandler handler) {
        std::scoped_lock lock{handlers_mutex_};
        error_handler_ = std::move(handler);
    }

    auto discover_folders() -> stdexec::task<models::DiscoveryResult> {
        models::DiscoveryResult result;

        auto root = co_await tpc::system::client::helpers::browse_async(*client_,
                                                                        opcua::NodeId{opcua::ObjectId::ObjectsFolder});

        auto target_channel_id = find_child(root, "ADC Channels", opcua::NodeClass::Object);

        if (!target_channel_id) {
            throw std::runtime_error{"Cannot find target channel ID"};
        }

        auto slots_result =
            co_await tpc::system::client::helpers::browse_async(*client_, std::move(*target_channel_id));

        auto slot_ids = find_slot_ids(slots_result);

        if (slot_ids.empty())
            throw std::runtime_error{"No slots found"};

        for (auto& slot_id : slot_ids) {
            auto slot_result = co_await tpc::system::client::helpers::browse_async(*client_, std::move(slot_id));

            append_channels(result.channels_state, slot_result);
        }

        int a = 9;
        co_return result;
    }

    std::expected<void, std::string> start_discovery() {
        auto task = stdexec::starts_on(stdexec::inline_scheduler{}, discover_folders());

        exec::start_detached(std::move(task));
        return {};
    }

    void subscribe_to_channels() { opcua::services::createSubscriptionAsync(); }

private:
    [[nodiscard]] static std::optional<opcua::NodeId>
    find_child(const opcua::BrowseResult& result, std::string_view name, opcua::NodeClass expected_class) {
        for (const auto& reference : result.references()) {

            if (!reference.isForward() || reference.nodeClass() != expected_class || !reference.nodeId().isLocal())
                continue;

            if (reference.browseName().name() == name)
                return reference.nodeId().nodeId();
        }

        return std::nullopt;
    };

    [[nodiscard]] static std::vector<opcua::NodeId> find_slot_ids(const opcua::BrowseResult& result) {
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

    [[nodiscard]] void append_channels(tpc::system::models::DiscoveryState& state,
                                       const opcua::BrowseResult&           slot_result) {
        for (const auto& reference : slot_result.references()) {
            if (!reference.isForward() || reference.nodeClass() != opcua::NodeClass::Variable
                || !reference.nodeId().isLocal()) {
                continue;
            }

            state.channels.push_back({std::string{reference.browseName().name()}, reference.nodeId().nodeId()});

            std::cout << reference.browseName().name() << "\n";
        }
    }

    void notification_token(opcua::IntegerId subscription_id, opcua::StatusChangeNotification& notification) {
        std::cerr << "Subscription " << subscription_id << " status: " << opcua::toString(notification.status())
                  << '\n';
    }

    void cancelation_token(opcua::IntegerId subscription_id) {
        if (subscription_id_ && *subscription_id_ == subscription_id) {
            subscription_id_.reset();
            monitored_item_ids_.clear();
            subscription_started_ = false;
        }
    }

    void completion_token(opcua::CreateSubscriptionResponse& response) {
        try {
            response.responseHeader().serviceResult().throwIfBad();

            subscription_id_ = response.subscriptionId();

            std::cout << "Subscription created, id = " << *subscription_id_
                      << ", publishing interval = " << response.revisedPublishingInterval() << " ms\n";

        } catch (const std::exception& error) {
            subscription_started_ = false;

            std::cerr << "Create subscription failed: " << error.what() << '\n';
        }
    }

private:
    std::unique_ptr<opcua::Client> client_;
    std::string                    endpoint_;

    bool session_state_{false};
    bool discovery_started_{false};
    bool subscription_started_{false};

    std::optional<opcua::IntegerId> subscription_id_;
    std::vector<opcua::IntegerId>   monitored_item_ids_;

    std::mutex handlers_mutex_;
    // UpdateHandler update_handler_;
    ErrorHandler error_handler_;

    std::atomic_bool running_{false};
    std::atomic_bool stop_requested_{false};

    exec::static_thread_pool opcua_pool_;
    exec::static_thread_pool processing_pool_;

private:
    const uint16_t polling_interval_ms_{50};
};

} // namespace tpc::system::tpc_client