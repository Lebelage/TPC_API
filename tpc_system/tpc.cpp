#include "tpc_system/tpc.hpp"

#include <expected>
#include <format>
#include <iostream>
#include <optional>
#include <span>
#include <unordered_map>
#include <utility>

#include "tpc_analytics/analytics_manager/analytics_manager.hpp"
#include "tpc_analytics/models/basis_models.hpp"
#include "tpc_analytics/models/three_dimension_models.hpp"
#include "tpc_core/definitions/client_definitions.hpp"
#include "tpc_system/models/data.hpp"
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

    for(auto& frame : result.value()){
        auto callibrtion = models::HallCalibrationCollection::find(frame.first);

        if(!callibrtion)
            continue;
            // return std::nullopt;

        frame.second = millivolts_to_gauss(frame.second, *callibrtion);
    }

    return std::move(result.value());
}

auto TPC::calculate_field_3d(std::vector<analytics::models::Measurement> measurements) -> void {
    if (measurements.empty())
        return;

    impl_->analytics_manager_.calculate_svd_coefficients(measurements, 0);
    // impl_->analytics_manager_.calculate_field(std::array<const std::size_t, __DIMENSION> components, double radius,
    // double z_length)
}

#pragma endregion

#pragma region Private Initialization

double TPC::volts_to_gauss(double voltage_volts, const models::HallCalibration& calibration) noexcept {
    const double voltage_mv = voltage_volts * 1000.0;

    return calibration.k * (voltage_mv - calibration.v0_mv);
}

double TPC::millivolts_to_gauss(double voltage_mv, const models::HallCalibration& calibration) noexcept {
    return calibration.k * (voltage_mv - calibration.v0_mv);
}

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
