module;
#include <open62541pp/client.hpp>
#include <stdexec/__detail/__task.hpp>

#include "exec/static_thread_pool.hpp"
export module tpc.system.client;
import std;

import tpc.system.models.data;
import tpc.system.client.subscription;

export namespace tpc::system::client {

class Client {
public:
    [[nodiscard]] static std::expected<std::unique_ptr<Client>, std::string> create(std::string endpoint);

public:
    ~Client() = default;

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
    Client(Client&&) = delete;
    Client& operator=(Client&&) = delete;

private:
    Client(std::string endpoint);

public:
    std::expected<bool, std::string> connect_async();

    void stop();

    bool is_running() const;

private:
    [[nodiscard]] auto initialize_opcua_handlers() -> std::expected<void, std::string>;

    [[nodiscard]] auto initialize_monitored_items() -> std::expected<void, std::string>;

    [[nodiscard]] auto create_subscription(models::DiscoveryResult& discovery) -> stdexec::task<void>;

    [[nodiscard]] auto discover_folders() -> stdexec::task<models::DiscoveryResult>;

    [[nodiscard]] auto start_discovery() -> std::expected<void, std::string>;

    [[nodiscard]] auto find_child(const opcua::BrowseResult& result,
                                  std::string_view name,
                                  opcua::NodeClass expected_class) -> std::optional<opcua::NodeId>;

    [[nodiscard]] auto find_slot_ids(const opcua::BrowseResult& result) -> std::vector<opcua::NodeId>;

    [[nodiscard]] auto append_channels(tpc::system::models::DiscoveryResult& result,
                                       const opcua::BrowseResult& slot_result) -> void;

private:
    std::unique_ptr<opcua::Client> client_;
    std::unique_ptr<tpc::system::client::Subscription> subscription_;
    std::string endpoint_;

    bool session_state_{false};
    bool discovery_started_{false};
    bool subscription_started_{false};

    std::optional<opcua::IntegerId> subscription_id_;
    std::vector<opcua::IntegerId> monitored_item_ids_;

    std::vector<opcua::NodeId> ids_;
    std::mutex handlers_mutex_;
    // UpdateHandler update_handler_;
    // ErrorHandler error_handler_;

    std::atomic_bool running_{false};
    std::atomic_bool stop_requested_{false};

    exec::static_thread_pool opcua_pool_;
    exec::static_thread_pool processing_pool_;

    models::DiscoveryResult channels_info;

private:
    const uint16_t polling_interval_ms_{50};
};
} // namespace tpc::system::client