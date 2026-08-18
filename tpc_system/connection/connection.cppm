module;

#include <exec/start_detached.hpp>
#include <exec/static_thread_pool.hpp>
#include <open62541pp/client.hpp>
#include <open62541pp/services/view.hpp>
#include <open62541pp/types.hpp>
#include <open62541pp/ua/nodeids.hpp>
#include <open62541pp/ua/types.hpp>
#include <stdexec/__detail/__task.hpp>
#include <stdexec/execution.hpp>

export module tpc.system.connection;

import std;

export namespace tpc::system
{
    // namespace adapters = tpc::system::connection::async_adapters;
    //
    // class Connection final
    // {
    // private:
    //     struct DiscoveryResult
    //     {
    //         tpc::system::models::DiscoveryState channels_state;
    //         std::vector<std::string>            detector_names;
    //     };
    //
    // public:
    //     [[nodiscard]] static std::expected<std::unique_ptr<Connection>, std::string> create(std::string_view endpoint)
    //     {
    //         try
    //         {
    //             return std::unique_ptr<Connection>{new Connection{endpoint}};
    //         }
    //         catch (const std::exception& error)
    //         {
    //             return std::unexpected{std::format("OPC UA connection creation failed: {}", error.what())};
    //         }
    //     }
    //
    //     ~Connection() = default;
    //
    //     Connection(const Connection&)            = delete;
    //     Connection& operator=(const Connection&) = delete;
    //     Connection(Connection&&)                 = delete;
    //     Connection& operator=(Connection&&)      = delete;
    //
    //     [[nodiscard]] std::expected<void, std::string> connect_async()
    //     {
    //         if (endpoint_.empty())
    //             return std::unexpected{"Endpoint is empty"};
    //
    //         if (!client_)
    //             return std::unexpected{"Client is not initialized"};
    //
    //         try
    //         {
    //             client_->connectAsync(endpoint_);
    //             return {};
    //         }
    //         catch (const std::exception& error)
    //         {
    //             return std::unexpected{std::format("Failed to connect: {}", error.what())};
    //         }
    //     }
    //
    //     [[nodiscard]] std::expected<void, std::string> disconnect_async()
    //     {
    //         if (!client_)
    //             return std::unexpected{"Client is not initialized"};
    //
    //         try
    //         {
    //             client_->disconnectAsync();
    //             return {};
    //         }
    //         catch (const std::exception& error)
    //         {
    //             return std::unexpected{std::format("Failed to disconnect: {}", error.what())};
    //         }
    //     }
    //
    //     [[nodiscard]] std::expected<void, std::string> subscribe_handlers()
    //     {
    //         if (!client_)
    //             return std::unexpected{"Client is not initialized"};
    //
    //         try
    //         {
    //             client_->onSessionActivated(
    //                 [this]
    //                 {
    //                     session_state_ = true;
    //                     std::cout << "Session activated\n";
    //                     start_discovery();
    //                 });
    //
    //             client_->onSessionClosed(
    //                 [this]
    //                 {
    //                     session_state_     = false;
    //                     discovery_started_ = false;
    //                     channels_.clear();
    //                     detector_names_.clear();
    //
    //                     std::cout << "Session closed\n";
    //                 });
    //
    //             client_->onInactive([] { std::cout << "Client inactive\n"; });
    //
    //             return {};
    //         }
    //         catch (const std::exception& error)
    //         {
    //             return std::unexpected{std::format("Failed to subscribe handlers: {}", error.what())};
    //         }
    //     }
    //
    //     [[nodiscard]] const std::vector<tpc::system::models::Channel>& channels() const noexcept { return channels_; }
    //
    //     [[nodiscard]] const std::vector<std::string>& detector_names() const noexcept { return detector_names_; }
    //
    //     void run() { client_->run(); }
    //
    // private:
    //     explicit Connection(std::string_view endpoint) : endpoint_(endpoint), client_(std::make_unique<opcua::Client>()) {}
    //
    //     // Objects → ADC Channels → Slot N → W1R/W1F/W1Z/etc.
    //     auto discover_all() -> stdexec::task<DiscoveryResult>
    //     {
    //         std::cout << std::this_thread::get_id() << std::endl;
    //
    //         auto root = co_await adapters::browse_async(*client_, opcua::NodeId{opcua::ObjectId::ObjectsFolder});
    //
    //         auto adc_channels_id = find_child(root, "ADC Channels", opcua::NodeClass::Object);
    //
    //         if (!adc_channels_id)
    //             throw std::runtime_error{"ADC Channels not found"};
    //
    //         auto slots_result = co_await adapters::browse_async(*client_, std::move(*adc_channels_id));
    //
    //         auto slot_ids = find_slot_ids(slots_result);
    //
    //         if (slot_ids.empty())
    //             throw std::runtime_error{"No slots found"};
    //
    //         DiscoveryResult result;
    //
    //         for (auto& slot_id : slot_ids)
    //         {
    //             auto slot_result = co_await adapters::browse_async(*client_, std::move(slot_id));
    //
    //             auto names = find_detector_names(slot_result);
    //
    //             result.detector_names.insert(
    //                 result.detector_names.end(), std::make_move_iterator(names.begin()), std::make_move_iterator(names.end()));
    //
    //             append_channels(result.channels_state, slot_result);
    //         }
    //
    //         std::ranges::sort(result.detector_names);
    //         const auto [first_duplicate, last] = std::ranges::unique(result.detector_names);
    //
    //         result.detector_names.erase(first_duplicate, last);
    //
    //         co_return result;
    //     }
    //
    //     void start_discovery()
    //     {
    //         if (!client_ || !session_state_ || discovery_started_)
    //             return;
    //
    //         discovery_started_ = true;
    //
    //         auto pipeline = stdexec::starts_on(stdexec::inline_scheduler{}, discover_all()) | stdexec::continues_on(pool_.get_scheduler())
    //
    //                         | stdexec::then(
    //                             [this](DiscoveryResult result)
    //                             {
    //                                 channels_ = std::move(result.channels_state.channels);
    //
    //                                 detector_names_ = std::move(result.detector_names);
    //
    //                                 discovery_started_ = false;
    //
    //                                 std::cout << "Detectors: " << detector_names_.size() << '\n';
    //
    //                                 for (const auto& name : detector_names_)
    //                                     std::cout << "- " << name << '\n';
    //
    //                                 std::cout << "Channels: " << channels_.size() << '\n';
    //
    //                                 for (const auto& channel : channels_)
    //                                 {
    //                                     std::cout << "- " << channel.name << ": " << opcua::toString(channel.node_id) << '\n';
    //                                 }
    //                             })
    //
    //                         | stdexec::upon_error(
    //                             [this](std::exception_ptr error) noexcept
    //                             {
    //                                 discovery_started_ = false;
    //
    //                                 try
    //                                 {
    //                                     std::rethrow_exception(error);
    //                                 }
    //                                 catch (const std::exception& ex)
    //                                 {
    //                                     std::cerr << "Discovery failed: " << ex.what() << '\n';
    //                                 }
    //                             });
    //
    //         exec::start_detached(std::move(pipeline));
    //     }
    //
    //     [[nodiscard]] static std::optional<opcua::NodeId> find_child(const opcua::BrowseResult& result,
    //                                                                  std::string_view           name,
    //                                                                  opcua::NodeClass           expected_class)
    //     {
    //         for (const auto& reference : result.references())
    //         {
    //             if (!reference.isForward() || reference.nodeClass() != expected_class || !reference.nodeId().isLocal())
    //             {
    //                 continue;
    //             }
    //
    //             if (reference.browseName().name() == name)
    //                 return reference.nodeId().nodeId();
    //         }
    //
    //         return std::nullopt;
    //     }
    //
    //     [[nodiscard]] static std::vector<opcua::NodeId> find_slot_ids(const opcua::BrowseResult& result)
    //     {
    //         std::vector<opcua::NodeId> ids;
    //
    //         for (const auto& reference : result.references())
    //         {
    //             if (!reference.isForward() || reference.nodeClass() != opcua::NodeClass::Object || !reference.nodeId().isLocal())
    //             {
    //                 continue;
    //             }
    //
    //             if (reference.browseName().name().starts_with("Slot "))
    //                 ids.push_back(reference.nodeId().nodeId());
    //         }
    //
    //         return ids;
    //     }
    //
    //     [[nodiscard]] static std::vector<std::string> find_detector_names(const opcua::BrowseResult& slot_result)
    //     {
    //         std::set<std::string> unique_names;
    //
    //         for (const auto& reference : slot_result.references())
    //         {
    //             if (!reference.isForward() || reference.nodeClass() != opcua::NodeClass::Variable || !reference.nodeId().isLocal())
    //             {
    //                 continue;
    //             }
    //
    //             const std::string channel_name{reference.browseName().name()};
    //
    //             // Python server: W1R/W1F/W1Z или E6R/E6F/E6Z.
    //             // Последний символ — компонент R/F/Z.
    //             if (channel_name.size() == 3)
    //                 unique_names.insert(channel_name.substr(0, 2));
    //         }
    //
    //         return {unique_names.begin(), unique_names.end()};
    //     }
    //
    //     static void append_channels(tpc::system::models::DiscoveryState& state, const opcua::BrowseResult& slot_result)
    //     {
    //         for (const auto& reference : slot_result.references())
    //         {
    //             if (!reference.isForward() || reference.nodeClass() != opcua::NodeClass::Variable || !reference.nodeId().isLocal())
    //             {
    //                 continue;
    //             }
    //
    //             state.channels.push_back({std::string{reference.browseName().name()}, reference.nodeId().nodeId()});
    //         }
    //     }
    //
    // private:
    //     std::string endpoint_;
    //
    //     bool session_state_{false};
    //     bool discovery_started_{false};
    //
    //     std::vector<tpc::system::models::Channel> channels_;
    //     std::vector<std::string>                  detector_names_;
    //
    //     std::unique_ptr<opcua::Client> client_;
    //
    //     exec::static_thread_pool pool_{2};
    // };
}  // namespace tpc::system