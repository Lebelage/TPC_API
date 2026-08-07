export module tpc.analytics.analytics_manager;
import std;

import tpc.utilities.header_function;
import tpc.third_party.eigen;
import tpc.analytics.models.three_dimension_model;
import tpc.analytics.svd.basis;
import tpc.analytics.svd_solver;
import tpc.core.definitions.analytics_definitions;
export namespace tpc::analytics
{
    using namespace tpc::third_party;
    using namespace tpc::analytics::models;
    using namespace tpc::core::definitions;

    template <typename... Args>
    class AnalyticsManager final
    {
        using TargetFunction = utilities::header_function<double(std::size_t, Args...)>;

    public:
        static std::expected<AnalyticsManager, std::string> create(Basis<Args...>&& basis)
        {
            if (basis.functions.empty())
                return std::unexpected("Basis functions collection is empty");

            if (basis.modes == 0)
                return std::unexpected("Basis modes count must be greater than zero");

            return AnalyticsManager{std::move(basis)};
        }

    private:
        AnalyticsManager(Basis<Args...>&& basis) : basis_(std::make_unique<Basis<Args...>>(std::move(basis))) {};

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

            if (!basis_ && basis_->functions.empty())
                return std::unexpected("Basis is incorrect!");

            auto result = SVDSolver::SolveSvd<Measurement, Args...>(measurements, basis_->functions, basis_->modes, threshold);

            if (!result)
                return std::unexpected(result.error());

            stored_coefficients_ = std::move(result.value());

            return {};
        }

        std::expected<models::FieldComponents, std::string> evaluate_for_point(std::span<const double> coordinates) const
        {
            const auto dimension = basis_->get_arity();

            if (coordinates.size() != dimension)
                return std::unexpected(std::format("Dimension mismatch: expected {}, but got {}", dimension, coordinates.size()));

            std::array<double, dimension> components{};

            for (std::size_t mode = 0; mode < basis_->modes; ++mode)
            {
                for (std::size_t i = 0; i < dimension; ++i)
                {
                    components[i] += stored_coefficients_[mode] * basis_->functions[i].invoke_from_span(mode, coordinates);
                }
            }

            return models::FieldComponents{.components = std::move(components), .coordinate_type = CoordinateType::Cylindric};
        }

        std::expected<void, std::string> calculate_field_cylindric(std::span<const double> components, double radius, double z_length)
        {
            if (!basis_)
                return std::unexpected("Basis is not initialized");

            if (components.empty())
                return std::unexpected("Components array is empty");

            const auto dimension = basis_->get_arity();

            if (dimension != __DIMENSION)
                return std::unexpected("Basis dimension mismatch");

            field_data_ = FieldCollection(basis_->get_arity(), CoordinateType::Cylindric, CoordinateType::Cylindric);

            const std::size_t count_r   = std::max<std::size_t>(1, components[0]);
            const std::size_t count_phi = std::max<std::size_t>(1, components[1]);
            const std::size_t count_z   = std::max<std::size_t>(1, components[2]);

            const std::size_t capacity = components[0] * components[1] * components[2];

            field_data_->Reserve(capacity);

            std::vector<double> coordinates_buffer;
            std::vector<double> field_buffer;

            coordinates_buffer.reserve(capacity * dimension);
            field_buffer.reserve(capacity * dimension);

            const double z_start = -z_length * 0.5;

            const double dr   = (count_r > 1) ? radius / (static_cast<double>(count_r - 1)) : 0.0;
            const double dphi = (count_phi > 1) ? (2 * std::numbers::pi / static_cast<double>(count_r - 1)) : 0.0;
            const double dz   = (count_z > 1) ? z_length / (static_cast<double>(count_z - 1)) : 0.0;

            std::array<const double, dimension> d_components = {dr, dphi, dz};

            calculate_field_kernel(components, d_components, z_start, field_buffer, coordinates_buffer);

            field_data_.value().InsertCoordinatesRange(coordinates_buffer);
            field_data_.value().InsertFieldRange(field_buffer);
        }

        std::vector<models::Measurement> GenerateTPCSensorMeasurements()
        {
            constexpr double R_SENSOR     = 1.0;
            constexpr double Z_ENDS[]     = {-1.5, 1.5};
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

    private:
        std::expected<std::span<const double>, std::string> calculate_field_kernel(std::array<const std::size_t, __DIMENSION> grid,
                                                                                   std::array<const double, __DIMENSION>      d_components,
                                                                                   std::size_t                                z_start,
                                                                                   std::span<double>                          field_buffer,
                                                                                   std::span<double> coordinate_buffer)
        {
            const auto [count_r, count_phi, count_z] = grid;
            const auto [d_r, d_phi, d_z]             = d_components;

            const std::size_t point_count = count_r * count_phi * count_z;

            for (std::size_t id = 0; id < point_count; ++id)
            {
                const std::size_t i     = id % count_r;
                const std::size_t plane = id / count_r;
                const std::size_t j     = id % count_phi;
                const std::size_t k     = id / count_phi;

                const double r   = static_cast<double>(i) * d_r;
                const double phi = static_cast<double>(j) * d_phi;
                const double z   = static_cast<double>(z_start) + static_cast<double>(k) * d_z;

                std::array<double, __DIMENSION> coordinates{r, phi, z};

                auto result = evaluate_for_point(coordinates);
                if (!result)
                    return std::unexpected("TODO ERROR");

                const std::size_t offset = id * __DIMENSION;

                field_buffer[offset]     = result.value().components[0];
                field_buffer[offset + 1] = result.value().components[1];
                field_buffer[offset + 2] = result.value().components[2];

                coordinate_buffer[offset]     = r;
                coordinate_buffer[offset + 1] = phi;
                coordinate_buffer[offset + 2] = z;
            }
        }

    public:
        static std::array<std::size_t, __DIMENSION> create_grid(std::size_t nx, std::size_t ny, std::size_t nz)
        {
            return std::array<std::size_t, __DIMENSION>{nx, ny, nz};
        };

        static Basis<Args...> create_default_basis(utilities::header_function<double(std::size_t, Args...)> first,
                                                   utilities::header_function<double(std::size_t, Args...)> second,
                                                   utilities::header_function<double(std::size_t, Args...)> third,
                                                   std::size_t                                              modes)
        {
            Basis<Args...> basis{};

            basis.functions.push_back(first);
            basis.functions.push_back(second);
            basis.functions.push_back(third);
            basis.modes      = modes;
            basis.basis_type = CoordinateType::Cylindric;

            return basis;
        }

    private:
        std::unique_ptr<Basis<Args...>> basis_;
        std::optional<FieldCollection>  field_data_;

        eigen::VectorXd stored_coefficients_;
    };
}  // namespace tpc::analytics