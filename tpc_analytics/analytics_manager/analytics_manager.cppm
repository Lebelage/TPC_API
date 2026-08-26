module;
#include <expected>
#include <string>
#include <vector>
#include <format>
#include <numbers>
export module tpc.analytics.analytics_manager;

import tpc.utilities.header_function;
import tpc.third_party.eigen;
import tpc.analytics.models.three_dimension_model;
import tpc.analytics.svd.basis;
import tpc.analytics.svd_solver;
import tpc.core.definitions.analytics_definitions;

import tpc.analytics.output.vtk_field_exporter;
export namespace tpc::analytics
{
    using namespace tpc::third_party;
    using namespace tpc::analytics::models;
    using namespace tpc::core::definitions;

    // template <typename... Args>
    class AnalyticsManager final
    {
        using TargetFunction = utilities::header_function<double(std::size_t, double, double, double)>;

    public:
        static std::expected<AnalyticsManager, std::string> create(BasisCollection&& basis)
        {
            if (basis.empty())
                return std::unexpected("Basis functions collection is empty");

            if (basis.get_modes() == 0)
                return std::unexpected("Basis modes count must be greater than zero");

            return AnalyticsManager{std::move(basis)};
        }

    private:
        AnalyticsManager(BasisCollection&& basis) : basis_collection_(std::move(basis)) {};

    public:
        ~AnalyticsManager() = default;

        AnalyticsManager(const AnalyticsManager&)            = delete;
        AnalyticsManager& operator=(const AnalyticsManager&) = delete;

        AnalyticsManager(AnalyticsManager&&) noexcept            = default;
        AnalyticsManager& operator=(AnalyticsManager&&) noexcept = default;

    public:
        std::expected<void, std::string> calculate_svd_coefficients(std::span<const Measurement> measurements, float threshold = 0)
        {
            if (measurements.empty())
                return std::unexpected("Measurements collection is empty!");

            if (basis_collection_.empty())
                return std::unexpected("Basis is incorrect!");

            auto result = SVDSolver::solve_svd(measurements, basis_collection_.get_basis(), basis_collection_.get_modes(), threshold);

            if (!result)
                return std::unexpected(result.error());

            stored_coefficients_ = std::move(result.value());

            return {};
        }

        std::expected<models::FieldComponents, std::string> evaluate_for_point(std::span<const double> coordinates) const
        {
            const auto dimension = basis_collection_.get_arity();

            if (coordinates.size() != dimension)
                return std::unexpected(std::format("Dimension mismatch: expected {}, but got {}", dimension, coordinates.size()));

            std::vector<double> components(dimension, 0.0);

            for (std::size_t mode = 0; mode < basis_collection_.get_modes(); ++mode)
            {
                for (std::size_t i = 0; i < dimension; ++i)
                {
                    components[i] += stored_coefficients_[mode] * basis_collection_.get_basis()[i].invoke_from_span(mode, coordinates);
                }
            }

            return models::FieldComponents{.components = std::move(components), .coordinate_type = CoordinateType::Cylindric};
        }

        std::expected<void, std::string> calculate_field(std::array<const std::size_t, __DIMENSION> components,
                                                         double                                     radius,
                                                         double                                     z_length)
        {
            if (radius <= 0.0 || z_length <= 0.0)
                return std::unexpected("Radius and depth must be positive");

            field_data_ = FieldCollection(basis_collection_.get_arity(), CoordinateType::Cylindric, CoordinateType::Cylindric);

            calculate_grid_kernel(components, radius, z_length);

            std::vector<double> coordinates_buffer{};
            std::vector<double> field_buffer{};

            coordinates_buffer.reserve(field_data_->get_coordinates().size());

            coordinates_buffer.insert(
                coordinates_buffer.begin(), field_data_->get_coordinates().begin(), field_data_->get_coordinates().end());

            field_buffer.resize(field_data_->get_coordinates().size());

            calculate_field_kernel(coordinates_buffer, field_buffer);

            field_data_->insert_field_range(field_buffer);

            return {};
        }

        std::vector<models::Measurement> GenerateTPCSensorMeasurements()
        {
            constexpr double R_SENSOR     = 2.0;
            constexpr double Z_ENDS[]     = {-3.5, 3.5};
            constexpr double BZ_MAIN      = 5000.0;
            constexpr double B_TRANSVERSE = 0.5;

            std::vector<models::Measurement> measurements;
            measurements.reserve(12);

            for (double z : Z_ENDS)
            {
                for (int i = 0; i < 6; ++i)
                {
                    const double phi        = i * (std::numbers::pi / 3.0);
                    const double current_Br = z < 0.0 ? -B_TRANSVERSE : B_TRANSVERSE;

                    measurements.emplace_back(models::PointComponents{{R_SENSOR, phi, z}, models::CoordinateType::Cylindric},
                                              models::FieldComponents{{current_Br, 0.0, BZ_MAIN}, models::CoordinateType::Cylindric});
                }
            }

            return measurements;
        }

        std::expected<void, std::string> export_to_vtk()
        {
            if (!field_data_.has_value())
                return std::unexpected("Field data is not calculated, cannot export to VTK");

            if (field_data_->get_field().empty() || field_data_->get_coordinates().empty())
                return std::unexpected("Field data is empty, cannot export to VTK");

            // field_data_->transform_field(CoordinateType::Cartesian);
            // auto result = field_data_->transform_coordinates(CoordinateType::Cartesian);

            // if (!result)
            // return std::unexpected("Failed to transform coordinates to Cartesian coordinates");

            tpc::analytics::output::VtkFieldExporter::export_3d_field_to_vtk(
                field_data_.value().get_field(), field_data_.value().get_coordinates(), "output.vtk");
            return {};
        }

    private:
        std::expected<void, std::string> calculate_grid_kernel(std::array<const std::size_t, __DIMENSION> grid_dimension,
                                                               double                                     radius,
                                                               double                                     depth)
        {
            const std::size_t radial_count = grid_dimension[0];
            const std::size_t z_count      = grid_dimension[2];

            if (radial_count < 2 || z_count < 2)
                return std::unexpected("Grid dimensions are too small");

            if (radius <= 0.0 || depth <= 0.0)
                return std::unexpected("Radius and depth must be positive");

            const double step_xy = radius / static_cast<double>(radial_count - 1);

            const double step_y = step_xy * std::sqrt(3.0) * 0.5;

            const double dz             = depth / static_cast<double>(z_count - 1);
            const double radius_squared = radius * radius;

            std::vector<double> coordinates_buffer;

            for (std::size_t iz = 0; iz < z_count; ++iz)
            {
                const double z = -depth * 0.5 + static_cast<double>(iz) * dz;

                std::size_t row = 0;

                for (double y = -radius; y <= radius + step_y * 0.5; y += step_y, ++row)
                {
                    const double x_offset = (row % 2 == 0) ? 0.0 : step_xy * 0.5;

                    for (double x = -radius + x_offset; x <= radius + step_xy * 0.5; x += step_xy)
                    {
                        if (x * x + y * y > radius_squared)
                            continue;

                        const double r = std::hypot(x, y);

                        const double phi = r < std::numeric_limits<double>::epsilon() ? 0.0 : std::atan2(y, x);

                        coordinates_buffer.push_back(r);
                        coordinates_buffer.push_back(phi);
                        coordinates_buffer.push_back(z);
                    }
                }
            }

            field_data_->insert_coordinates_range(coordinates_buffer);

            return {};
        }

        /// Need to rework
        std::expected<void, std::string> calculate_field_kernel(std::span<const double> coordinates_buffer, std::span<double> field_buffer)
        {
            if (coordinates_buffer.size() % __DIMENSION != 0)
                return std::unexpected("Coordinate buffer has an invalid size");

            const std::size_t point_count = coordinates_buffer.size() / __DIMENSION;

            if (field_buffer.size() != point_count * __DIMENSION)
                return std::unexpected("Field buffer has an invalid size");

            for (std::size_t id = 0; id < point_count; ++id)
            {
                const std::size_t offset = id * __DIMENSION;

                const std::array<double, __DIMENSION> coordinates{
                    coordinates_buffer[offset],
                    coordinates_buffer[offset + 1],
                    coordinates_buffer[offset + 2],
                };

                auto result = evaluate_for_point(coordinates);
                if (!result)
                    return std::unexpected("Failed to calculate field at grid point");

                field_buffer[offset]     = result->components[0];
                field_buffer[offset + 1] = result->components[1];
                field_buffer[offset + 2] = result->components[2];
            }

            return {};
        }

    public:
        static std::array<const std::size_t, __DIMENSION> create_grid(std::size_t nx, std::size_t ny, std::size_t nz)
        {
            return std::array<const std::size_t, __DIMENSION>{nx, ny, nz};
        };

        static BasisCollection create_default_basis(TargetFunction first, TargetFunction second, TargetFunction third, std::size_t modes)
        {
            BasisCollection basis = BasisCollection::create(__DIMENSION, modes, CoordinateType::Cylindric).value();

            basis.add_back(std::move(first));
            basis.add_back(std::move(second));
            basis.add_back(std::move(third));

            return basis;
        }

    private:
        BasisCollection basis_collection_;

        std::optional<FieldCollection> field_data_;

        eigen::VectorXd stored_coefficients_;
    };
}  // namespace tpc::analytics