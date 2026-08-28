#pragma once
#include <expected>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "client/client.hpp"
#include "client/frame_receiver.hpp"
#include "event_handler.hpp"

namespace tpc::system {

using ReceivedItem = client::ReceivedItem;

class TPC {
public:
    [[nodiscard]] static std::expected<std::unique_ptr<TPC>, std::string> create(std::string_view endpoint);
    ~TPC() = default;

    TPC(const TPC&) = delete;
    TPC& operator=(const TPC&) = delete;

    TPC(TPC&&) noexcept = default;
    TPC& operator=(TPC&&) noexcept = default;

public:
    auto start_async() -> void;
    auto stop_async() -> void;
    auto is_running() -> bool;

    auto get_frame_request() -> std::optional<std::vector<ReceivedItem>>;

private:
    auto on_client_error(std::string) -> void;
    auto on_client_info(std::string) -> void;
    auto on_client_connection_state_changed(client::ConnectionState) -> void;

private:
    TPC(std::string endpoint);

    auto initialize_start_handlers() -> void;

private:
    void dispose();
public:
    utilities::event_handler<std::string> error_occurred_;
    utilities::event_handler<std::string> warning_occurred_;
    utilities::event_handler<std::string> info_occurred_;
    utilities::event_handler<client::ConnectionState> connection_state_changed_;

    std::unique_ptr<client::Client> client_;
};
} // namespace tpc::system