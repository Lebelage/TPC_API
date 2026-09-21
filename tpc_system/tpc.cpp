#include "tpc_system/tpc.hpp"

#include <expected>
#include <format>
#include <optional>
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
};

#pragma region Factory / Constructor

std::expected<std::unique_ptr<TPC>, std::string> TPC::create(std::string_view endpoint) {
    try {
        return std::unique_ptr<TPC>{new TPC(std::string{endpoint})};
    } catch (const std::exception& error) {
        return std::unexpected{
            std::format("[{}]: Failed to create TPC device: {}", core::definitions::CLIENT_ERROR, error.what())
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
        throw std::runtime_error{analytics_create_result.error()};

    impl_ = std::make_unique<AnalyticsImpl>(AnalyticsImpl{.analytics_manager_ = std::move(*analytics_create_result)});
}

TPC::~TPC() = default;

#pragma endregion

#pragma region Public Methods

auto TPC::start_async() -> void {
    auto result = client_->connect_async();

    if (!result)
        error_occurred_.invoke(result.error());
    else if (!*result)
        warning_occurred_.invoke("TPC client is already running");
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

    auto frame = std::move(*result);

    for (auto& [sensor_name, value] : frame) {
        const auto calibration = models::HallCalibrationCollection::find(sensor_name);

        if (!calibration)
            continue;

        value = millivolts_to_gauss(value, *calibration);
    }

    return frame;
}

auto TPC::calculate_field_async(std::vector<analytics::models::Measurement> measurements, std::array<size_t, tpc::core::definitions::DIMENSION> grid, double radius, double length) -> void {
    if (measurements.empty())
        return;

    bool calculation_already_in_progress = false;

    {
        std::lock_guard lock{field_worker_mutex_};

        if (field_calculation_in_progress_) {
            calculation_already_in_progress = true;
        } else {
            if (!field_worker_.joinable()) {
                field_worker_ = std::jthread([this](std::stop_token stop_token) {
                    field_worker_loop(stop_token);
                });
            }

            models::CalculationData calculation_data{
                .grid = std::move(grid),
                .measurements = std::move(measurements),
                .radius = radius,
                .length = length
            };

            pending_calculation_data_ = std::move(calculation_data);
            field_calculation_in_progress_ = true;
        }
    }

    if (calculation_already_in_progress) {
        warning_occurred_.invoke("Field calculation is already in progress");
        return;
    }

    field_worker_cv_.notify_one();
}

#pragma endregion

#pragma region Private Initialization

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

auto TPC::field_worker_loop(std::stop_token stop_token) -> void {
    while (!stop_token.stop_requested()) {
        models::CalculationData calculation_data;

        {
            std::unique_lock lock{field_worker_mutex_};
            const bool has_work = field_worker_cv_.wait(lock, stop_token, [this] {
                return pending_calculation_data_.has_value();
            });

            if (!has_work)
                return;

            calculation_data = std::move(*pending_calculation_data_);
            pending_calculation_data_.reset();
        }

        bool calculation_succeeded = false;
        std::string error;

        try {
            auto coefficients_result = impl_->analytics_manager_.calculate_svd_coefficients(calculation_data.measurements, 1e-4F);

            if (!coefficients_result) {
                error = std::move(coefficients_result.error());
            } else {
                auto field_result = impl_->analytics_manager_.calculate_field(calculation_data.grid, calculation_data.radius, calculation_data.length);

                if (!field_result)
                    error = std::move(field_result.error());
                else
                    calculation_succeeded = true;
            }
        } catch (const std::exception& exception) {
            error = exception.what();
        } catch (...) {
            error = "Unknown error during field calculation";
        }

        {
            std::lock_guard lock{field_worker_mutex_};
            field_calculation_in_progress_ = false;
        }

        if (!calculation_succeeded) {
            try {
                error_occurred_.invoke(std::format("Field calculation failed: {}", error));
            } catch (...) {
                // A user callback .
            }
        }

        try {
            field_was_calculated_.invoke(calculation_succeeded);
        } catch (...) {
            // A user callback.
        }
    }
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
