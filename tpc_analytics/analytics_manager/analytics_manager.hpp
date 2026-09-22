#pragma once

#include <cmath>

#include <array>
#include <expected>
#include <limits>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>

#include "third_party/eigen_module.hpp"
#include "tpc_analytics/models/three_dimension_models.hpp"
#include "tpc_analytics/output/vtk_field_exporter.hpp"
#include "tpc_analytics/svd/basis.hpp"
#include "tpc_analytics/svd/svd_solver.hpp"
#include "tpc_core/definitions/analytics_definitions.hpp"
#include "utilities/header_function.hpp"
namespace tpc::analytics {

class AnalyticsManager final {
    using TargetFunction = utilities::header_function<double(std::size_t, double, double, double)>;
    static constexpr std::size_t kDimension = core::definitions::DIMENSION;

public:
    class FieldReadView final {
        friend class AnalyticsManager;

        FieldReadView(std::shared_lock<std::shared_mutex>&& lock, const models::FieldCollection& field_data)
            : lock_(std::move(lock)), field_data_(&field_data) {}

    public:
        FieldReadView(FieldReadView&&) noexcept = default;
        FieldReadView& operator=(FieldReadView&&) noexcept = default;

        FieldReadView(const FieldReadView&) = delete;
        FieldReadView& operator=(const FieldReadView&) = delete;

        [[nodiscard]] std::span<const double> get_coordinates() const noexcept {
            return field_data_->get_coordinates();
        }

        [[nodiscard]] std::span<const double> get_field() const noexcept {
            return field_data_->get_field();
        }

    private:
        std::shared_lock<std::shared_mutex> lock_;
        const models::FieldCollection* field_data_;
    };

    static std::expected<AnalyticsManager, std::string> create(BasisCollection&& basis) {
        if (basis.empty())
            return std::unexpected("Basis functions collection is empty");

        if (basis.get_modes() == 0)
            return std::unexpected("Basis modes count must be greater than zero");

        return AnalyticsManager{std::move(basis)};
    }

private:
    AnalyticsManager(BasisCollection&& basis) : basis_collection_(std::move(basis)) {}

public:
    ~AnalyticsManager() = default;

    AnalyticsManager(const AnalyticsManager&) = delete;
    AnalyticsManager& operator=(const AnalyticsManager&) = delete;

    AnalyticsManager(AnalyticsManager&& other) noexcept
        : basis_collection_(std::move(other.basis_collection_)),
          field_data_(std::move(other.field_data_)),
          field_data_valid_(other.field_data_valid_),
          stored_coefficients_(std::move(other.stored_coefficients_)) {}

    AnalyticsManager& operator=(AnalyticsManager&& other) noexcept {
        if (this == &other)
            return *this;

        basis_collection_ = std::move(other.basis_collection_);
        field_data_ = std::move(other.field_data_);
        field_data_valid_ = other.field_data_valid_;
        stored_coefficients_ = std::move(other.stored_coefficients_);
        return *this;
    }

public:
    std::expected<void, std::string> calculate_svd_coefficients(
        std::span<models::Measurement> measurements, float threshold = 0
    ) {
        if (measurements.empty())
            return std::unexpected("Measurements collection is empty!");

        if (basis_collection_.empty())
            return std::unexpected("Basis is incorrect!");

        for (auto& measurement : measurements) {
            auto transform_result =
                measurement.point_components.transform_coordinates(models::CoordinateType::Cylindric);

            if (!transform_result)
                return std::unexpected(transform_result.error());
        }

        auto result = SVDSolver::solve_svd(
            std::span<const models::Measurement>{measurements},
            basis_collection_.get_basis(),
            basis_collection_.get_modes(),
            threshold
        );

        if (!result)
            return std::unexpected(result.error());

        std::unique_lock lock{coefficients_mutex_};
        stored_coefficients_ = std::move(*result);

        return {};
    }

    std::expected<models::FieldComponents, std::string> evaluate_for_point(
        std::array<double, kDimension> coordinates
    ) const {
        std::shared_lock lock{coefficients_mutex_};
        const auto modes = basis_collection_.get_modes();

        if (stored_coefficients_.size() != static_cast<Eigen::Index>(modes))
            return std::unexpected("SVD coefficients are not calculated");

        const auto basis = basis_collection_.get_basis();
        std::array<double, kDimension> components{};

        for (std::size_t mode = 0; mode < modes; ++mode) {
            for (std::size_t i = 0; i < kDimension; ++i) {
                components[i] += stored_coefficients_[static_cast<Eigen::Index>(mode)] *
                                 basis[i](mode, coordinates[0], coordinates[1], coordinates[2]);
            }
        }

        return models::FieldComponents{.components = components, .coordinate_type = models::CoordinateType::Cylindric};
    }

    std::expected<void, std::string> calculate_field(
        std::array<std::size_t, kDimension> grid_dimensions, double radius, double z_length
    ) {
        auto validation_result = validate(radius, z_length);

        if (!validation_result)
            return std::unexpected(validation_result.error());

        std::shared_lock coefficients_lock{coefficients_mutex_};

        if (stored_coefficients_.size() != static_cast<Eigen::Index>(basis_collection_.get_modes()))
            return std::unexpected("SVD coefficients are not calculated");

        std::unique_lock field_lock{field_data_mutex_};
        field_data_valid_ = false;

        if (!field_data_)
            field_data_.emplace(kDimension, models::CoordinateType::Cylindric, models::CoordinateType::Cylindric);
        else
            field_data_->clear();

        auto grid_result = calculate_grid_kernel(grid_dimensions, radius, z_length);

        if (!grid_result)
            return std::unexpected(grid_result.error());

        field_data_->resize_field_to_coordinates();
        auto field_result = calculate_field_kernel(field_data_->get_coordinates(), field_data_->get_mutable_field());

        if (!field_result)
            return std::unexpected(field_result.error());

        field_data_valid_ = true;
        return {};
    }

    [[nodiscard]] std::expected<FieldReadView, std::string> get_field_data() const {
        std::shared_lock lock{field_data_mutex_};

        if (!field_data_valid_ || !field_data_)
            return std::unexpected("Field data is not calculated");

        return FieldReadView{std::move(lock), *field_data_};
    }

    std::expected<void, std::string> export_to_vtk() {
        std::shared_lock lock{field_data_mutex_};

        if (!field_data_valid_ || !field_data_.has_value())
            return std::unexpected("Field data is not calculated, cannot export to VTK");

        if (field_data_->get_field().empty() || field_data_->get_coordinates().empty())
            return std::unexpected("Field data is empty, cannot export to VTK");

        return tpc::analytics::output::VtkFieldExporter::export_3d_field_to_vtk(
            field_data_->get_field(), field_data_->get_coordinates(), "output.vtk"
        );
    }

private:
    std::expected<void, std::string> calculate_grid_kernel(
        std::array<std::size_t, kDimension> grid_dimensions, double radius, double depth
    ) {
        const std::size_t x_count = grid_dimensions[0];
        const std::size_t y_count = grid_dimensions[1];
        const std::size_t z_count = grid_dimensions[2];

        if (x_count < 2 || y_count < 2 || z_count < 2)
            return std::unexpected("Grid dimensions are too small");

        if (radius <= 0.0 || depth <= 0.0)
            return std::unexpected("Radius and depth must be positive");

        const double step_x = radius / static_cast<double>(x_count - 1);
        const double step_y = radius / static_cast<double>(y_count - 1) * std::sqrt(3.0) * 0.5;

        const double dz = depth / static_cast<double>(z_count - 1);
        const double radius_squared = radius * radius;

        auto for_each_grid_point = [&](auto&& visitor) {
            for (std::size_t iz = 0; iz < z_count; ++iz) {
                const double z = -depth * 0.5 + static_cast<double>(iz) * dz;
                std::size_t row = 0;

                for (double y = -radius; y <= radius + step_y * 0.5; y += step_y, ++row) {
                    const double x_offset = (row % 2 == 0) ? 0.0 : step_x * 0.5;

                    for (double x = -radius + x_offset; x <= radius + step_x * 0.5; x += step_x) {
                        if (x * x + y * y <= radius_squared)
                            visitor(x, y, z);
                    }
                }
            }
        };

        std::size_t point_count = 0;
        for_each_grid_point([&](double, double, double) {
            ++point_count;
        });

        if (point_count > std::numeric_limits<std::size_t>::max() / kDimension)
            return std::unexpected("Grid is too large");

        field_data_->reserve(point_count * kDimension);
        for_each_grid_point([&](double x, double y, double z) {
            const double r = std::hypot(x, y);
            const double phi = r < std::numeric_limits<double>::epsilon() ? 0.0 : std::atan2(y, x);
            field_data_->append_coordinate(r, phi, z);
        });

        return {};
    }

    std::expected<void, std::string> calculate_field_kernel(
        std::span<const double> coordinates_buffer, std::span<double> field_buffer
    ) {
        if (coordinates_buffer.size() % kDimension != 0)
            return std::unexpected("Coordinate buffer has an invalid size");

        const std::size_t point_count = coordinates_buffer.size() / kDimension;

        if (field_buffer.size() != point_count * kDimension)
            return std::unexpected("Field buffer has an invalid size");

        const auto modes = basis_collection_.get_modes();
        const auto basis = basis_collection_.get_basis();

        for (std::size_t id = 0; id < point_count; ++id) {
            const std::size_t offset = id * kDimension;

            const std::array<double, kDimension> coordinates{
                coordinates_buffer[offset],
                coordinates_buffer[offset + 1],
                coordinates_buffer[offset + 2],
            };
            std::array<double, kDimension> components{};

            for (std::size_t mode = 0; mode < modes; ++mode) {
                const double coefficient = stored_coefficients_[static_cast<Eigen::Index>(mode)];

                for (std::size_t component = 0; component < kDimension; ++component) {
                    components[component] +=
                        coefficient * basis[component](mode, coordinates[0], coordinates[1], coordinates[2]);
                }
            }

            field_buffer[offset] = components[0];
            field_buffer[offset + 1] = components[1];
            field_buffer[offset + 2] = components[2];
        }

        return {};
    }


public:
    std::expected<void, std::string> prepare_vtk_data() {
        tpc::analytics::output::VtkFieldExporter::export_3d_field_to_vtk(field_data_.value().get_field(), field_data_.value().get_coordinates(), "field.vtk");
        return{};
    }

    static std::array<std::size_t, kDimension> create_grid(std::size_t nx, std::size_t ny, std::size_t nz) {
        return {nx, ny, nz};
    }

    static BasisCollection create_default_basis(
        TargetFunction first, TargetFunction second, TargetFunction third, std::size_t modes
    ) {
        auto basis_result = BasisCollection::create(kDimension, modes, models::CoordinateType::Cylindric);

        if (!basis_result)
            throw std::invalid_argument{basis_result.error()};

        auto basis = std::move(*basis_result);

        (void)basis.add_back(std::move(first));
        (void)basis.add_back(std::move(second));
        (void)basis.add_back(std::move(third));

        return basis;
    }

private:
    std::expected<void, std::string> validate(double radius, double z_length) const {
        const auto basis_dimension = basis_collection_.get_arity();

        if (basis_dimension != kDimension)
            return std::unexpected("Basis does not contain enough components");

        if (radius <= 0.0 || z_length <= 0.0)
            return std::unexpected("Radius and depth must be positive");

        return {};
    }

private:
    BasisCollection basis_collection_;

    std::optional<models::FieldCollection> field_data_;
    bool field_data_valid_{false};
    mutable std::shared_mutex field_data_mutex_;

    third_party::eigen::VectorXd stored_coefficients_;
    mutable std::shared_mutex coefficients_mutex_;
};
}  // namespace tpc::analytics
