#pragma once

#include <cstddef>
#include <expected>
#include <limits>
#include <span>
#include <string>

#include "tpc/analytics/eigen_module.hpp"
#include "tpc/analytics/concepts/measurement_concept.hpp"
#include "tpc/analytics/models/three_dimension_models.hpp"
#include "tpc/utilities/header_function.hpp"
namespace tpc::analytics {
class SVDSolver {
public:
    template <concepts::MeasurementConcept Measurement>
    static std::expected<third_party::eigen::VectorXd, std::string> solve_svd(
        std::span<const Measurement> measurements,
        std::span<const utilities::header_function<double(std::size_t, double, double, double)>> basis_functions,
        std::size_t modes,
        double threshold
    ) {
        if (measurements.empty())
            return std::unexpected("Measurement collection is empty");

        if (basis_functions.empty())
            return std::unexpected("Basis functions collection is empty");

        if (modes == 0)
            return std::unexpected("Modes count must be greater than zero");

        if (threshold < 0.0)
            return std::unexpected("SVD threshold cannot be negative");

        const std::size_t field_dimension = measurements.front().get_point_components().size();

        if (field_dimension == 0)
            return std::unexpected("Measurement dimension must be greater than zero");

        if (basis_functions.size() != field_dimension)
            return std::unexpected("Number of basis functions must equal field dimension");

        if (measurements.size() > static_cast<std::size_t>(std::numeric_limits<Eigen::Index>::max()) / field_dimension)
            return std::unexpected("Measurement matrix is too large");

        if (modes > static_cast<std::size_t>(std::numeric_limits<Eigen::Index>::max()))
            return std::unexpected("Modes count is too large");

        const auto rows = static_cast<Eigen::Index>(measurements.size() * field_dimension);
        const auto columns = static_cast<Eigen::Index>(modes);
        third_party::eigen::MatrixXd matrix(rows, columns);
        third_party::eigen::VectorXd values_vector(rows);

        for (std::size_t measurement_index = 0; measurement_index < measurements.size(); ++measurement_index) {
            const auto coordinates = measurements[measurement_index].get_point_components();
            const auto values = measurements[measurement_index].get_field_components();

            if (coordinates.size() != field_dimension || values.size() != field_dimension)
                return std::unexpected("Measurement dimensions are inconsistent");

            for (std::size_t component = 0; component < field_dimension; ++component) {
                const auto row = static_cast<Eigen::Index>(field_dimension * measurement_index + component);
                values_vector(row) = values[component];

                for (std::size_t mode = 0; mode < modes; ++mode) {
                    matrix(row, static_cast<Eigen::Index>(mode)) =
                        basis_functions[component].invoke_from_span(mode, coordinates);
                }
            }
        }

        third_party::eigen::JacobiSVD<third_party::eigen::MatrixXd> svd(
            matrix, third_party::eigen::ComputeThinU | third_party::eigen::ComputeThinV
        );
        svd.setThreshold(threshold);

        return svd.solve(values_vector);
    }
};
}  // namespace tpc::analytics
