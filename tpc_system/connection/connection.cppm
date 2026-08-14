module;

#include <exec/static_thread_pool.hpp>
#include <open62541pp/client.hpp>
#include <open62541pp/services/attribute_highlevel.hpp>
#include <open62541pp/services/monitoreditem.hpp>
#include <open62541pp/services/subscription.hpp>
#include <open62541pp/services/view.hpp>
#include <open62541pp/types.hpp>
#include <open62541pp/ua/nodeids.hpp>
#include <open62541pp/ua/types.hpp>
#include <string_view>

export module tpc.system.connection;
import std;
import tpc.system.models.data;

export namespace tpc::system
{
    class Connection final
    {
    public:
        [[nodiscard]] static std::expected<std::unique_ptr<Connection>, std::string> create(std::string_view endpoint)
        {
            try
            {
                auto connection = std::unique_ptr<Connection>(new Connection(endpoint));

                return connection;
            }
            catch (const std::exception& error)
            {
                return std::unexpected{std::format("OPC UA connection failed: {}", error.what())};
            }
        }

        ~Connection() = default;

        Connection(const Connection&)            = delete;
        Connection& operator=(const Connection&) = delete;

        Connection(Connection&&) noexcept            = default;
        Connection& operator=(Connection&&) noexcept = default;

        void process_events() {}

    private:
        Connection(std::string_view endpoint) : endpoint_(endpoint) { client_ = std::make_unique<opcua::Client>(); }

    public:
        std::expected<void, std::string> connect_async()
        {
            if (endpoint_.empty())
                return std::unexpected("Endpoint is empty");

            if (!client_)
                return std::unexpected("Failed to create OPC UA client");

            try
            {
                client_->connectAsync(endpoint_);
            }
            catch (std::exception& ex)
            {
                return std::unexpected(std::format("Failed to connect to OPC UA server: {}", ex.what()));
            }

            return {};
        }

        std::expected<void, std::string> disconnect_async()
        {
            if (!client_)
                return std::unexpected("Client is not initialized");

            try
            {
                client_->disconnectAsync();
            }
            catch (std::exception& ex)
            {
                return std::unexpected(std::format("Failed to disconnect from OPC UA server: {}", ex.what()));
            }

            return {};
        }

        std::expected<void, std::string> subscribe_handlers()
        {
            if (!client_)
                return std::unexpected("Client is not initialized");

            try
            {
                client_->onSessionActivated(
                    [this]()
                    {
                        session_state_ = true;
                        std::cout << "Session activated" << std::endl;
                        discover_channels_async();
                    });

                client_->onSessionClosed([this]() { std::cout << "Session closed" << std::endl; });

                client_->onInactive([this]() { std::cout << "Client inactive" << std::endl; });
            }
            catch (std::exception& ex)
            {
                return std::unexpected(std::format("Failed to subscribe to handlers: {}", ex.what()));
            }

            return {};
        }

    public:
        std::expected<bool, std::string> is_connected()
        {
            if (!client_)
                return std::unexpected("Client is not initialized");

            auto result = client_->isConnected();
            return result;
        }

        void run() { client_->run(); }

    private:
        template <typename Callback>
        void browse_async(opcua::NodeId node_id, Callback&& callback)
        {
            const opcua::BrowseDescription description{
                std::move(node_id), opcua::BrowseDirection::Forward, opcua::ReferenceTypeId::References};

            opcua::services::browseAsync(*client_, description, 0, std::forward<Callback>(callback));
        }

        void discover_channels_async()
        {
            if (!client_ || !session_state_ || discovery_started_)
                return;

            discovery_started_ = true;

            browse_async(opcua::NodeId{opcua::ObjectId::ObjectsFolder},
                         [this](opcua::BrowseResult& result)
                         {
                             const auto channels_id = find_child(result, "ADC Channels", opcua::NodeClass::Object);

                             if (!channels_id)
                             {
                                 std::cerr << "ADC Channels not found" << std::endl;
                                 discovery_started_ = false;
                                 return;
                             }

                             discover_slots_async(*channels_id);
                         });
        }

        void discover_slots_async(const opcua::NodeId& adc_channels_id)
        {
            browse_async(
                adc_channels_id,
                [this](opcua::BrowseResult& result)
                {
                    auto state = std::make_shared<tpc::system::models::DiscoveryState>();

                    std::vector<opcua::NodeId> slot_ids;

                    for (const auto& reference : result.references())
                    {
                        if (!reference.isForward() || reference.nodeClass() != opcua::NodeClass::Object || !reference.nodeId().isLocal())
                            continue;

                        const auto name = reference.browseName().name();

                        if (!name.starts_with("Slot "))
                            continue;

                        slot_ids.push_back(reference.nodeId().nodeId());
                    }

                    if (slot_ids.empty())
                    {
                        std::cerr << "No slots found" << std::endl;
                        discovery_started_ = false;
                        return;
                    }

                    state->pending_slots = slot_ids.size();

                    for (const auto& slot_id : slot_ids)
                    {
                        browse_async(
                            slot_id,
                            [this, state](opcua::BrowseResult& slot_result)
                            {
                                for (const auto& reference : slot_result.references())
                                {
                                    if (!reference.isForward() || reference.nodeClass() != opcua::NodeClass::Variable
                                        || !reference.nodeId().isLocal())
                                    {
                                        continue;
                                    }

                                    state->channels.push_back({std::string{reference.browseName().name()}, reference.nodeId().nodeId()});
                                }

                                --state->pending_slots;

                                if (state->pending_slots != 0)
                                    return;

                                channels_ = std::move(state->channels);

                                std::cout << "Discovered " << channels_.size() << " channels\n";

                                for (const auto& channel : channels_)
                                {
                                    std::cout << "- " << channel.name << ": " << opcua::toString(channel.node_id) << '\n';
                                }

                                if (channels_.empty())
                                {
                                    discovery_started_ = false;
                                    return;
                                }

                                // subscribe_to_channels_async();
                            });
                    }
                });
        }

        [[nodiscard]] static std::optional<opcua::NodeId> find_child(const opcua::BrowseResult& result,
                                                                     std::string_view           name,
                                                                     opcua::NodeClass           expected_class)
        {
            for (const auto& reference : result.references())
            {
                if (!reference.isForward() || reference.nodeClass() != expected_class || !reference.nodeId().isLocal())
                {
                    continue;
                }

                if (reference.browseName().name() == name)
                    return reference.nodeId().nodeId();
            }

            return std::nullopt;
        }

    private:
        std::string_view endpoint_{};
        bool             session_state_{false};
        bool             discovery_started_{false};

        std::vector<tpc::system::models::Channel> channels_;

        std::unique_ptr<opcua::Client> client_;
    };
}  // namespace tpc::system