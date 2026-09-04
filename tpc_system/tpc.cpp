#include "tpc.hpp"

#include <format>
#include <iostream>
#include <utility>

import tpc.core.definitions.client_definitions;
import tpc.analytics.analytics_manager;
import tpc.analytics.models.basis_models;
import tpc.analytics.models.three_dimension_model;

namespace tpc::system {

struct AnalyticsImpl {
    analytics::AnalyticsManager analytics_manager_;
    analytics::models::Measurement measurement_;
};

#pragma region Factory / Constructor

std::expected<std::unique_ptr<TPC>, std::string> TPC::create(std::string_view endpoint) {
    try {
        return std::unique_ptr<TPC>{new TPC(std::string{endpoint})};
    } catch (const std::exception& error) {
        return std::unexpected{
            std::format("[{}]: Failed to create TPC device: {}", core::definitions::CLIENT_ERROR__, error.what())
        };
    } catch (...) {
        return std::unexpected{"Failed to create TPC: unknown error"};
    }
}

TPC::TPC(std::string endpoint) {
    auto result = client::Client::create(std::move(endpoint));

    if (!result) {
        throw std::runtime_error{result.error()};
    }

    client_ = std::move(*result);
    initialize_start_handlers();

    auto basis = analytics::AnalyticsManager::create_default_basis(
        analytics::models::DefaultBasisR,
        analytics::models::DefaultBasisPhi,
        analytics::models::DefaultBasisZ,
        analytics::models::DEFAULT_MODES_COUNT_
    );

    auto analytics_create_result = tpc::analytics::AnalyticsManager::create(std::move(basis));

    if (!analytics_create_result)
        return;

    impl_ = std::make_unique<AnalyticsImpl>(
        AnalyticsImpl{.analytics_manager_ = std::move(analytics_create_result.value())}
    );
}

TPC::~TPC() = default;

TPC::TPC(TPC&&) noexcept = default;
TPC& TPC::operator=(TPC&&) noexcept = default;

#pragma endregion

#pragma region Public Methods

auto TPC::start_async() -> void {
    client_->connect_async();
}

auto TPC::stop_async() -> void {
    client_->stop();
}

auto TPC::is_running() const -> bool {
    return client_->is_running();
}

auto TPC::get_frame_request() -> std::optional<std::unordered_map<std::string, double>> {
    auto result = client_->get_frame();

    if (!result) {
        return std::nullopt;
    }

    return std::move(result.value());
}

auto TPC::calculate_field_3d(std::span<double> sensors_values, std::span<double> sensors_pos) -> void {
    if (sensors_values.empty() || sensors_pos.empty())
        return;

    if ((sensors_values.size() != sensors_pos.size()) || sensors_pos.size() % 3 != 0 || sensors_values.size() % 3 != 0)
        return;

    std::vector<analytics::models::Measurement> measurements;

    for (std::size_t i = 0; i < sensors_values.size(); i += 3) {
        auto measurement = analytics::models::Measurement{
            .point_components =
                {.components = {sensors_pos[i], sensors_pos[i + 1], sensors_pos[i + 2]},
                                   .coordinate_type = analytics::CoordinateType::Cylindric},
            .field_components = {
                                   .components = {sensors_values[i], sensors_values[i + 1], sensors_values[i + 2]},
                                   .coordinate_type = analytics::CoordinateType::Cylindric}
        };

        measurements.emplace_back(measurement);
    }

    impl_->analytics_manager_.calculate_svd_coefficients(measurements, 0);
}

#pragma endregion

#pragma region Private Initialization

auto TPC::initialize_start_handlers() -> void {
    (void)client_->error_occurred_.subscribe([this](const std::string& err) {
        on_client_error(err);
    });

    (void)client_->info_occurred_.subscribe([this](const std::string& info) {
        on_client_info(info);
    });

    (void)client_->connection_state_changed_.subscribe([this](client::ConnectionState state) {
        on_client_connection_state_changed(state);
    });

    (void)client_->initialization_data_received_.subscribe([this](const models::DiscoveryResult& discovery_result) {
        on_client_initialization_data_received(discovery_result);
    });
}

#pragma endregion

#pragma region Event Handlers

auto TPC::on_client_error(const std::string& err) -> void {
    error_occurred_.invoke(err);
}

auto TPC::on_client_info(const std::string& info) -> void {
    info_occurred_.invoke(info);
}

auto TPC::on_client_connection_state_changed(client::ConnectionState state) -> void {
    connection_state_changed_.invoke(state);
}

auto TPC::on_client_initialization_data_received(const models::DiscoveryResult& discovery_result) -> void {
    initialization_data_received_.invoke(discovery_result);
}

#pragma endregion

}  // namespace tpc::system
