#include "tpc.hpp"

#include <format>
#include <iostream>

import tpc.core.definitions.client_definitions;

namespace tpc::system {

#pragma region Fabric/Constructor

std::expected<std::unique_ptr<TPC>, std::string> TPC::create(std::string_view endpoint) {
    try {
        return std::unique_ptr<TPC>{new TPC(std::string{endpoint})};
    } catch (const std::exception& error) {
        return std::unexpected{
            std::format("[{}]: Failed to create TPC device: {}", core::definitions::CLIENT_ERROR__, error.what())};
    } catch (...) {
        return std::unexpected("Failed to create TPC: unknown error");
    };
}

TPC::TPC(std::string endpoint) {
    auto result = client::Client::create(std::move(endpoint));

    if (!result) {
        throw std::runtime_error{result.error()};
    }

    client_ = std::move(*result);

    initialize_start_handlers();
}

#pragma endregion

#pragma region Public methods

auto TPC::start_async() -> void {
    client_->connect_async();
}

auto TPC::stop_async() -> void {
    client_->stop();
}

auto TPC::is_running() -> bool {
    return client_->is_running();
}

auto TPC::get_frame_request() -> std::optional<std::vector<ReceivedItem>> {
    auto result = client_->get_frame();

    if (!result)
        return std::nullopt;

    return result.value();
}

#pragma endregion

#pragma region Private methods

auto TPC::initialize_start_handlers() -> void {
    client_->error_occurred_.subscribe([this](std::string err) {
        on_client_error(err);
    });
    client_->info_occurred_.subscribe([this](std::string info) {
        on_client_info(info);
    });
    client_->connection_state_changed_.subscribe([this](client::ConnectionState state) {
        on_client_connection_state_changed(state);
    });
}

#pragma endregion

#pragma region Hendlers

auto TPC::on_client_error(std::string err) -> void {
    std::cout << err << "\n";
}

auto TPC::on_client_info(std::string info) -> void {
    std::cout << info << "\n";
}

auto TPC::on_client_connection_state_changed(client::ConnectionState state) -> void {
    connection_state_changed_.invoke(state);
}

#pragma endregion
} // namespace tpc::system