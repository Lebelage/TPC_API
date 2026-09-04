#pragma once

#include <expected>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "client/client.hpp"
#include "client/frame_receiver.hpp"
#include "event_handler.hpp"

namespace tpc::system {

using ReceivedItem = models::ReceivedItem;

struct AnalyticsImpl;
/**
 * @class TPC
 * @brief High-level facade controller managing communication with the TPC
 * hardware subsystem.
 *
 * Encapsulates client lifecycle management, asynchronous connection workflows,
 * telemetry frame acquisition, and event dispatching for upstream UI/Services.
 */
class TPC {
public:
    /**
     * @brief Factory method that instantiates and initializes a TPC controller.
     *
     * Creates the underlying client instance, verifies connection parameters,
     * and binds event handlers.
     *
     * @param[in] endpoint The OPC UA server connection URL (e.g.,
     * "opc.tcp://127.0.0.1:4840").
     * @return std::expected<std::unique_ptr<TPC>, std::string>
     *         - On success: A `std::unique_ptr<TPC>` owning the controller.
     *         - On failure: A `std::unexpected` containing the diagnostic error
     * message.
     */
    [[nodiscard]] static std::expected<std::unique_ptr<TPC>, std::string> create(std::string_view endpoint);

    /**
     * @brief Default virtual-safe destructor.
     */
    ~TPC();

    TPC(const TPC&) = delete;
    TPC& operator=(const TPC&) = delete;

    TPC(TPC&&) noexcept;
    TPC& operator=(TPC&&) noexcept;

public:
    /**
     * @brief Initiates an asynchronous connection routine to the target endpoint.
     */
    auto start_async() -> void;

    /**
     * @brief Halts background worker routines and disconnects from the endpoint.
     */
    auto stop_async() -> void;

    /**
     * @brief Checks whether the client communication loop is actively executing.
     * @return True if the communication pipeline is running, false otherwise.
     */
    [[nodiscard]] auto is_running() const -> bool;

    /**
     * @brief Requests the latest snapshot of sensor telemetry values.
     * @return `std::optional` containing a map of sensor names to numeric values,
     *         or `std::nullopt` if no frame is currently available.
     */
    [[nodiscard]] auto get_frame_request() -> std::optional<std::unordered_map<std::string, double>>;

    [[nodiscard]] auto calculate_field_3d(std::span<double> sensors_values, std::span<double> sensors_pos) -> void;

private:
    auto on_client_error(const std::string& err) -> void;
    auto on_client_info(const std::string& info) -> void;
    auto on_client_initialization_data_received(const models::DiscoveryResult& result) -> void;
    auto on_client_connection_state_changed(client::ConnectionState state) -> void;

    explicit TPC(std::string endpoint);
    auto initialize_start_handlers() -> void;

    void dispose();

public:
    /** @brief Emitted when a subsystem error occurs. */
    utilities::event_handler<std::string> error_occurred_;

    /** @brief Emitted when a subsystem warning is raised. */
    utilities::event_handler<std::string> warning_occurred_;

    /** @brief Emitted for informational diagnostic messages. */
    utilities::event_handler<std::string> info_occurred_;

    /** @brief Emitted when the client connection state transitions. */
    utilities::event_handler<client::ConnectionState> connection_state_changed_;

    /** @brief Emitted when initial sensor discovery metadata is received. */
    utilities::event_handler<models::DiscoveryResult> initialization_data_received_;

private:
    std::unique_ptr<struct AnalyticsImpl> impl_;
    std::unique_ptr<client::Client> client_;
};

}  // namespace tpc::system