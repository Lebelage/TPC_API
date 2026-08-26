#pragma once
#include <string>
#include <vector>
#include <expected>
#include <optional>
#include <memory>

#include "client/client.hpp"
#include "client/frame_receiver.hpp"
#include "event_handler.hpp"

// export module tpc.system;
// import tpc.system.client;
// import tpc.system.client.frame_receiver;

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

private:
    TPC(std::string endpoint);

    auto initialize_start_handlers() -> void;

private:
    std::unique_ptr<client::Client> client_;
};
} // namespace tpc::system